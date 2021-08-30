const MonacoEditorPlugin = require('monaco-editor-webpack-plugin')

module.exports = {
  outputDir: 'docs',
  publicPath: '/ece220-fa20-zjui/lc3webtool/',
  configureWebpack: {
      plugins: [
          new MonacoEditorPlugin({
              languages: [],
              features: []
          })
      ]
  },
  transpileDependencies: [
    "vuetify"
  ]
}