import ARKit
import AVFoundation
import ExpoModulesCore

final class ARSessionStore {
  static let shared = ARSessionStore()

  let session = ARSession()
  weak var view: ARSCNView?
  var latestFrame: ARFrame?
  var latestViewProjection = [Double](repeating: 0, count: 16)
  var tracking = "initializing"
  var onFrame: (([String: Any]) -> Void)?

  private var lastEmit: CFTimeInterval = 0

  func emitFrame(force: Bool = false) {
    guard let arView = view, let frame = latestFrame else { return }
    latestFrame = frame

    let now = CACurrentMediaTime()
    if !force && now - lastEmit < 0.1 { return }
    lastEmit = now

    let size = arView.bounds.size
    guard size.width > 0, size.height > 0 else { return }

    let orientation = arView.window?.windowScene?.interfaceOrientation ?? .portrait
    let viewMatrix = frame.camera.viewMatrix(for: orientation)
    let projection = frame.camera.projectionMatrix(
      for: orientation,
      viewportSize: size,
      zNear: 0.01,
      zFar: 300
    )
    let combined = simd_mul(projection, viewMatrix)

    var rowMajor = [Double](repeating: 0, count: 16)
    for column in 0..<4 {
      for row in 0..<4 {
        rowMajor[row * 4 + column] = Double(combined[column][row])
      }
    }
    latestViewProjection = rowMajor
    tracking = trackingName(frame.camera.trackingState)

    onFrame?([
      "viewProjection": rowMajor,
      "tracking": tracking,
      "width": Double(size.width),
      "height": Double(size.height),
    ])
  }

  func trackingName(_ state: ARCamera.TrackingState) -> String {
    switch state {
    case .normal:
      return "normal"
    case .notAvailable:
      return "unavailable"
    case .limited(let reason):
      switch reason {
      case .initializing: return "initializing"
      case .excessiveMotion: return "excessive motion"
      case .insufficientFeatures: return "low features"
      case .relocalizing: return "relocalizing"
      @unknown default: return "limited"
      }
    }
  }
}

final class VolumeARView: ExpoView, ARSessionDelegate, ARSCNViewDelegate {
  let arView = ARSCNView(frame: .zero)

  var active: Bool = true {
    didSet {
      if active {
        resume()
      } else {
        ARSessionStore.shared.session.pause()
      }
    }
  }

  required init(appContext: AppContext? = nil) {
    super.init(appContext: appContext)

    arView.automaticallyUpdatesLighting = true
    arView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    arView.session.delegate = self
    arView.delegate = self
    addSubview(arView)

    if AVCaptureDevice.authorizationStatus(for: .video) == .notDetermined {
      AVCaptureDevice.requestAccess(for: .video) { _ in }
    }

    ARSessionStore.shared.view = arView
    ARSessionStore.shared.session.delegate = self
    resume()
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    arView.frame = bounds
  }

  private func resume() {
    let configuration = ARWorldTrackingConfiguration()
    configuration.planeDetection = [.horizontal, .vertical]
    configuration.environmentTexturing = .automatic
    if ARWorldTrackingConfiguration.supportsFrameSemantics(.sceneDepth) {
      configuration.frameSemantics.insert(.sceneDepth)
    }
    ARSessionStore.shared.session.run(configuration, options: [.resetTracking, .removeExistingAnchors])
    ARSessionStore.shared.emitFrame(force: true)
  }

  func session(_ session: ARSession, didUpdate frame: ARFrame) {
    ARSessionStore.shared.latestFrame = frame
    ARSessionStore.shared.emitFrame()
  }

  func session(_ session: ARSession, didFailWithError error: Error) {
    ARSessionStore.shared.tracking = "error"
  }

  func sessionWasInterrupted(_ session: ARSession) {
    ARSessionStore.shared.tracking = "interrupted"
  }
}

enum ARHit {
  static func worldPoint(at point: CGPoint) -> (position: simd_float3, realism: Double)? {
    guard let arView = ARSessionStore.shared.view else { return nil }

    let types: [ARHitTestResult.ResultType] = [
      .existingPlaneUsingExtent,
      .estimatedVerticalPlane,
      .estimatedHorizontalPlane,
      .existingPlane,
      .featurePoint,
    ]

    if let hit = arView.hitTest(point, types: types).first {
      let realism: Double
      switch hit.type {
      case .existingPlaneUsingExtent: realism = 0.95
      case .existingPlane: realism = 0.9
      case .estimatedVerticalPlane, .estimatedHorizontalPlane: realism = 0.7
      case .featurePoint: realism = 0.45
      default: realism = 0.4
      }
      return (simd_make_float3(hit.worldTransform.columns.3), realism)
    }

    guard let frame = ARSessionStore.shared.session.currentFrame else { return nil }
    let transform = frame.camera.transform
    let origin = simd_make_float3(transform.columns.3)
    let forward = -simd_normalize(simd_make_float3(transform.columns.2))
    return (origin + forward * 5.0, 0.2)
  }
}
