const { getDefaultConfig } = require('expo/metro-config');

const config = getDefaultConfig(__dirname);

config.resolver.blockList = [
  /.*\/modules\/volume-ar\/core\/build\/.*/,
  /.*\/modules\/volume-ar\/core\/build$/,
];

module.exports = config;
