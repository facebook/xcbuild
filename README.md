
<h1 align="center">
  <!--<img src="xcbuild_logo.jpg" alt="Facebook xcbuild" />-->
  xcbuild
</h1>

**xcbuild** is an Xcode-compatible build tool with the goal of providing faster builds, better documentation of the build process and running on multiple platforms (in particular OS X and Linux)

### Why use xcbuild? 

<table>
  <tr>
    <th colspan="2">Features</th>
    <th rowspan="10"></th>
    <th colspan="3">Performance</th>
  </tr>
  <tr><td>:rocket:</td><td>Blazing fast incremental builds</td><th rowspan="3"></th><th rowspan="3"><code>xcodebuild</code></th><th rowspan="3">xcbuild + Ninja</th></tr>
  <tr><td>:book:</td><td>Documents the Xcode build process</td></tr>
  <tr><td>:link:</td><td>Builds Xcode projects and workspaces</td></tr>
  <tr><td>:hatching_chick:</td><td>Supports Swift apps and frameworks</td><th rowspan="3">Clean Build</th><td rowspan="3">30.103s</td><td rowspan="3">25.122s</td></tr>
  <tr><td>:sparkles:</td><td>Tools and libraries for Xcode projects</td></tr>
  <tr><td>:gift_heart:</td><td>Fully compatible with <a href="https://github.com/supermarin/xcpretty">xcpretty</a></td></tr>
  <tr><td>:tophat:</td><td>Uses <a href="https://martine.github.io/ninja">Ninja</a> and <a href="https://github.com/apple/swift-llbuild">llbuild</a></td><th rowspan="3">Incremental Build</th><td rowspan="3">2.190s</td><td rowspan="3">0.046s :zap:</td></tr>
  <tr><td>:octocat:</td><td>Open source under the BSD license</td></tr>
  <tr><td>:penguin:</td><td>Builds on Linux</td></tr>
</table>

### xcbuild and other build tools

[xctool](https://github.com/facebook/xctool) | [Buck](https://github.com/facebook/buck) | [xcpretty](https://github.com/supermarin/xcpretty)
----|----|---
xcbuild and [xctool](https://github.com/facebook/xctool) are both Xcode-compatible build systems. We plan on slowly deprecating xctool's build support but keep it as a great way to run tests. | Facebook's main build system is [Buck](https://buckbuild.com). Buck has a stronger architecture and advanced features like artifact caching while having a much simpler build format. If you have a new project, it's highly recommended. | xcbuild works great with [xcpretty](https://github.com/supermarin/xcpretty). Pipe the output from xcbuild to xcpretty the same way as you would from `xcodebuild`.

## Building

- Xcode 7 or later, on OS X.
- [CMake](http://www.cmake.org) and [Ninja](https://martine.github.io/ninja/) (or [llbuild](https://github.com/apple/swift-llbuild)). 
  - With [Homebrew](http://brew.sh/): `brew install cmake ninja`

```sh
git clone https://github.com/facebook/xcbuild
cd xcbuild
git submodule update --init
make
```

Build output will be in the `build` directory. Run xcbuild with `./build/xcbuild`.

## Usage

The command line options are compatible with [xcodebuild](https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man1/xcodebuild.1.html).

```
xcbuild -workspace Example.xcworkspace -scheme Example
```

### Using Ninja (or llbuild)

To switch to the significantly faster [Ninja](https://martine.github.io/ninja/) executor:

```sh
xcbuild -executor ninja [-workspace Example.xcworkspace ...]
```

```
Wrote meta-ninja: ~/Library/Developer/Xcode/DerivedData/.../Build/Intermediates/build.ninja
```

Copy the path to the `meta-ninja` and run

```sh
ninja -f ~/Library/Developer/Xcode/DerivedData/.../Build/Intermediates/build.ninja
``` 

## Contributing

xcbuild actively welcomes contributions from the community. If you're interested in contributing, be sure to check out the [contributing guide](https://github.com/facebook/xcbuild/blob/master/CONTRIBUTING.md). It includes some tips for getting started in the codebase, as well as important information about the code of conduct, license, and CLA.

## Thanks

xcbuild is built on build system documentation from the community. In particular, thanks to these people for their writing:

 - [Samantha Marshall](http://pewpewthespells.com)
 - [Damien Bobillot](http://maxao.free.fr/xcode-plugin-interface/)
 - [Michele Titolo](http://michele.io)
 - [Laurent Etiemble](http://www.monobjc.net/xcode-project-file-format.html)
 - [Apple Developer](https://developer.apple.com/legacy/library/documentation/DeveloperTools/Conceptual/XcodeBuildSystem/Xcode_Build_System.pdf)

Third-party licenses are listed in the `LICENSE` document.
