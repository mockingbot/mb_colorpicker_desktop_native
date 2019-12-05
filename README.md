# mb_colorpicker_desktop_native

An color picker for Win and Mac with pure native platform API


## structure
the file:
- source:
  * `src/` - source code for both darwin & win32 
  * `res/` - image resource
- output: `_dst/` - binary output
- test: `test/` - test Electron app, use binary from `_dst/`


## build
if you have `npm` installed:
```shell script
npm run build-win32
npm run build-darwin
```
or check `scripts` in [./package.json](./package.json) for command


## electron test
first setup electron test with: 
```shell script
npm run electron-test-install
```

then run:
```shell script
npm run electron-test-start
```
