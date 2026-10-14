# Lua views

Views are XML templates rendered by the native Lua template engine. Literal
markup is preserved, `${...}` expressions are evaluated as Lua, and lines
beginning with `%` are Lua code:

```xml
<view>
  <section title="${model.title}">
    % for i,3 do
    <text>${i}</text>
    % end
  </section>
</view>
```

Count-form loops such as `% for i,3 do` run from 1 through 3. Other `%` lines
are passed to Lua unchanged. Templates can read `method`, `uri`, `action`,
`data`, and `model`; `data` and `model` are Lua tables. Standard Lua libraries
are available to the template.
