# Views framework

The views framework will allow for extending Spacify
and will be using a V8 based JavaScript engine to allow integrations

Each view is written in JS

````javascript

import render from '@spacify/ui'

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
