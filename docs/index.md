# Documentation
## Introduction

Most apps of today is sharing the same archietype, with **features**, **entities** and **plugins** but despite the same UX structure, they reimplement the wheel eveytime, and bundles reimplementation of the same concepts into massive web and app bundles weighting of multiple GB in storage and enormous ram consumption.
With the growing geopolitical tension, and the advent of AI, prices of RAM is going to skyrocket, while modern operating systems such macOS and Windows 11 is taking over 10 GB in its core.

Our Space UI concept is to build a new cross platforn application stack from the ground that instead of forcing app developers to reinvent everything, with thousands of libraries, that is based on the archietypes of apps.

Meet Space UI which is a new kind of app runtime where fundamental principles of an app structure already built in the UI framework, and the developer serves Concepts that define the app instead of bla bla bla of creating toolbar and menu.

Instead of apps based on massive JetPack, Swifr or HTML5, it will be defined in concepta such

````xml
<app>
<entities>

<features>
<feature id="start">
<view>
<section id="overview">
<text>Hello World</text>
</section>
</view>
</feature>
</features>
</app>
````

Or in YAML

````yaml
app:
  - title: Test
  features:
    start:
````
