Pod::Spec.new do |s|
  s.name           = 'VolumeAR'
  s.version        = '0.1.0'
  s.summary        = 'AR corner pinning and volume measurement'
  s.description    = 'ARKit session, world anchors, ray hit testing and the shared volume core.'
  s.author         = 'Volume'
  s.homepage       = 'https://example.local/volume'
  s.license        = 'MIT'
  s.platforms      = { :ios => '15.1' }
  s.swift_version  = '5.9'
  s.source         = { :path => '.' }
  s.static_framework = true

  s.dependency 'ExpoModulesCore'

  core = '../core'

  s.source_files = [
    '*.{h,m,mm,swift}',
    "#{core}/src/*.cpp",
    "#{core}/include/**/*.h",
  ]

  s.pod_target_xcconfig = {
    'HEADER_SEARCH_PATHS' => "\"$(PODS_TARGET_SRCROOT)/#{core}/include\"",
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17',
    'DEFINES_MODULE' => 'YES',
  }
end
