Refactor to this pattern:

We want to work with abstraction at high level so views can share characteristics.

1. Rename the nomenklure title to 'name' in all entities, because it is the new standard.
2. Add a 'version' field to Recording.
3. We want a root abstract entity called Node which have getName() and getVersion().

Then, rename the Artist nomenclature to Creator. 

Then we have an abstract Content class which Recording and Release are subclassing from it, which adds getCreatorCredits() seCreatorCredits() which is a list of CreatorCredit like in Release and Recording that is abstracted to CreatorCredit<T extends Content>, where ReleaseCreatorCredits and RecordingCreatorCredits are subclassing (ReleaseCreatorCredits extends CreatorCredits<Release>, RecordingCreatorCredits<Relase> etc.), and the getCreatorCredits and setCreatorCredits in Content subclasses is then abstracted to the correct subclass like above.
This lays the foundation for expanding to other verticals and content types later (Show/Season/Episode, Video, Game etc.) which also can share the same infrastructure like the Buy/Purchase button because new verticals and music is inheriting from Media and Content.

Create a new class, ContentCollection<T extends Content> which is the base for the upcoming playlist, with ContentCollectionRow<T extends Content> which will be the baes class for Track extends ContentCollectionRow<T extends Content> and recording.

Make a new generic class Release<T extends Content> subclassing Content, andm move today's existing implementation of Release subclass of Release<Content>, MusicRelease and make Release a generic class Release<T extends Content> , to support future release types (VideoRelease, GameRelease etc.).  

4. Detach the file path from the Recording node, into a on own table, and remove the file path field from the Recording table altogether. which references the Recording node, in order to make all playback, streaming and purchase types treated equally, so local files is as much a 'music service' as YouTube.

5. Implement a basic playlist service, which stores playlist in database with rows that consists of playlist_entity which cross references different Content type like PlaylistRow<T extends Content> which rows of multiple content can be mixed, and in db playlist_rows with index, playlist name, and the content link is stored as URI: eg. with the identifier/name lookup model as we outlined before with spacify: uri when viewing the playlist. The playlist should be a subclass of ContentCollection<Content> where PlaylistRow is a subclass of CotnentCollectionRow<Content>.

This is a large change but will make it more flexible, as we will add support for multiple content types which can be mixed in playlists.