Write an RFC for the musik: uri scheme

Scheme name: musik
Status: Provisional
Applications/protocols that use this scheme name: Bungalow
Contact: Alexander Forselius <drsounds@gmail.com>
Change controller: Alexander Forselius <drsounds@gmail.com>
Scheme syntax:musik:isrc:<ISRC Code> - for particular recording
musik:iswc:<ISWC code> - for particular work
musik:upc:<UPC code> - for particular release
musik:isni:<ISNI code> - for particular artist/creator
musik:ipi:<IPI number> - IPI number
musik:genre:<URL encoded slug of genre name>
musik:mood:<URL encoded slug of mood>
musik:artist:<URL encoded artist name>[:release:<URL encoded release name>[[:track:<number 0-99>]:name:<URL encoded track name>[:version:<URL encoded Version title>]]]
music:track?name=<URL encoded name>&artist_name=<URL encoded artist name>&release_name=<URL encoded release name>&number=0-99&version=<URL encoded version title> 

With updated documentation about this in the docs and  addsupport for this uri type in the uri resolver of the music app / concept and in playlist/library/drag n drop etc.