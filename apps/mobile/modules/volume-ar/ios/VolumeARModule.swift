import ARKit
import AVFoundation
import ExpoModulesCore

public class VolumeARModule: Module {
  private var cornerCounter = 0

  public func definition() -> ModuleDefinition {
    Name("VolumeAR")

    Events("onFrame", "onCornerUpdate")

    OnCreate {
      ARSessionStore.shared.onFrame = { [weak self] payload in
        self?.sendEvent("onFrame", payload)
      }
    }

    OnDestroy {
      ARSessionStore.shared.onFrame = nil
      ARSessionStore.shared.session.pause()
    }

    AsyncFunction("start") {
      if AVCaptureDevice.authorizationStatus(for: .video) == .authorized {
        ARSessionStore.shared.emitFrame(force: true)
      }
    }

    AsyncFunction("stop") {
      ARSessionStore.shared.session.pause()
    }

    Function("reset") {
      self.cornerCounter = 0
      if let frame = ARSessionStore.shared.session.currentFrame {
        for anchor in frame.anchors {
          ARSessionStore.shared.session.remove(anchor: anchor)
        }
      }
    }

    AsyncFunction("addCorner") { (px: Double, py: Double) -> [String: Any] in
      let point = CGPoint(x: px, y: py)
      guard let hit = ARHit.worldPoint(at: point) else {
        return ["id": "", "x": 0.0, "y": 0.0, "z": 0.0, "confidence": 0.0, "rayCount": 0]
      }

      self.cornerCounter += 1
      let id = "c\(self.cornerCounter)"

      if let frame = ARSessionStore.shared.session.currentFrame {
        let anchor = ARAnchor(transform: frame.camera.transform)
        ARSessionStore.shared.session.add(anchor: anchor)
      }

      let position = hit.position
      return [
        "id": id,
        "x": Double(position.x),
        "y": Double(position.y),
        "z": Double(position.z),
        "confidence": hit.realism,
        "rayCount": hit.realism > 0.6 ? 3 : 1,
      ]
    }

    AsyncFunction("pickPoint") { (px: Double, py: Double) -> [String: Any] in
      let point = CGPoint(x: px, y: py)
      guard let hit = ARHit.worldPoint(at: point) else {
        return ["id": "", "x": 0.0, "y": 0.0, "z": 0.0, "confidence": 0.0, "rayCount": 0]
      }
      let position = hit.position
      return [
        "id": "p\(UUID().uuidString.prefix(6))",
        "x": Double(position.x),
        "y": Double(position.y),
        "z": Double(position.z),
        "confidence": hit.realism,
        "rayCount": hit.realism > 0.6 ? 3 : 1,
      ]
    }

    AsyncFunction("getViewProjection") { () -> [Double] in
      if ARSessionStore.shared.latestViewProjection.allSatisfy({ $0 == 0 }) {
        ARSessionStore.shared.emitFrame(force: true)
      }
      return ARSessionStore.shared.latestViewProjection
    }

    Function("polygonArea") { (points: [Double]) -> Double in
      return VolumeCoreBridge.polygonArea(points as NSArray)
    }

    Function("computeMeasurement") {
      (footprint: [Double], mode: String, manualHeight: Double, base: [Double],
       top: [Double], roof: [Double], up: [Double]) -> [String: Any] in
      return VolumeCoreBridge.compute(
        withFootprint: footprint as NSArray,
        mode: mode,
        manualHeight: manualHeight,
        base: base as NSArray,
        top: top as NSArray,
        roof: roof as NSArray,
        up: up as NSArray
      )
    }

    View(VolumeARView.self) {
      Prop("active") { (view: VolumeARView, active: Bool) in
        view.active = active
      }
    }
  }
}
