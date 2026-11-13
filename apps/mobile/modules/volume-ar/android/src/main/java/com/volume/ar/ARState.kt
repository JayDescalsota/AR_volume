package com.volume.ar

import com.google.ar.core.Anchor
import com.google.ar.core.Frame
import com.google.ar.core.Session

object ARState {
  @Volatile var session: Session? = null
  @Volatile var latestFrame: Frame? = null
  @Volatile var latestViewProjection = DoubleArray(16)
  @Volatile var tracking = "initializing"
  @Volatile var active = false
  var onFrame: ((Map<String, Any>) -> Unit)? = null

  val anchors = mutableListOf<Anchor>()

  fun clearAnchors() {
    for (anchor in anchors) {
      anchor.detach()
    }
    anchors.clear()
  }
}
