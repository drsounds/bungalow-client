# Views framework

The views framework will allow for extending Bungalow
and will be using a V8 based JavaScript engine to allow integrations

Each view is written in JS

````javascript

import render from '@bungalow/ui'

function Page() {
   return render(
      'view',
      {
         flex: 1
      },
      render(
         'text',
         {},
         'Hello World!'
      )
   )
}

````
