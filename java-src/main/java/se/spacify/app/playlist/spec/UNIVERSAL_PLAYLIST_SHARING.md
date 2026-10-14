Spec for universal playlist sharing link (UPS):

To create an universal decentralized URI for a playlist:

````text
    spacify:federated:user:<mastodon_user_name eg. @username@domain.com:playlist:<base62 encoded data of (<base62 encoded name>:description:<base62 encoded description>[:image:<base64_image_url>]:uris:[<base 62 encoded uri eg. spacify:isrc:<isrc>,  spacify:artist:<artist_name>:release:<release_name>:track:<track_number>:recording:<recording_name>:version:[ <recording_version>],https:* etc.])>
````

also translable to

````
    https://<tbd_base_domain.tld>/user/<mastodon_user_name>/playlist/<base_62 encoded of predicates>
````

whose uri parts will be resolved by URI resolver in Spacify. Due to the 2,048 limitation, the amount of entries will be limited depending of amount of content. 

This link can be created by clicking a button