In the spider app, (se/spacify/app) I've made stubs with comments for the spider system, a web server like template processing language, in Lua, which takes a Controller which process a Request with the lua template String defined in the comments of the process method of the controller.

The controller will utilizing Spider that integrates with a Java library that integrates Lua engine, with the preprocessing directives implemented in a separate class here like the example outlined above.

= Template file

Lua views are like ASP:

````xml
<view>
    <page id="overview" title="Overview">
        <text>${os.date("%Y-%m-D")}</text>
        % for i,10 do 
        <text>${i}</text>
        % end
        <button onclick="refresh">Refresh</button>
    </page>
</view>
````

Will in the views/SpiderView.java be render to a se.spider.controls.Control view, and re-render upon postback, depending on a controller that defines the template, data to process and return the org.w3c.dom.Element tree.