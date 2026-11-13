// @ts-check

/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
  docs: [
    'intro',
    'background',
    {
      type: 'category',
      label: 'Getting started',
      collapsed: false,
      items: ['getting-started/building', 'getting-started/project-layout'],
    },
    {
      type: 'category',
      label: 'Architecture',
      collapsed: false,
      items: [
        'architecture/overview',
        'architecture/aspects',
        'architecture/features-and-concepts',
        'architecture/navigation',
        'architecture/search',
        'architecture/playback',
        'architecture/plugins',
        'architecture/data',
      ],
    },
    {
      type: 'category',
      label: 'Space UI framework',
      items: [
        'space/overview',
        'space/elements-and-xul',
        'space/templates',
        'space/uri-store',
        'space/skins',
        'space/backends',
      ],
    },
    {
      type: 'category',
      label: 'Apps',
      items: [
        'apps/bundles',
        'apps/native-apps',
        'apps/html5-apps',
        'apps/sites-and-bookmarks',
      ],
    },
    {
      type: 'category',
      label: 'Reference',
      items: ['reference/uri-schemes', 'reference/settings-and-files'],
    },
    {
      type: 'category',
      label: 'Specifications',
      link: {type: 'doc', id: 'specs/index'},
      items: ['specs/rfc-0001-universal-playlist-sharing', 'specs/rfc-0002-musik-uri-scheme'],
    },
    'roadmap',
  ],
  javaClient: [
    {
      type: 'category',
      label: 'Java reference client',
      collapsed: false,
      link: {type: 'doc', id: 'java-client/index'},
      items: [
        'java-client/architecture',
        'java-client/plugin-system',
        'java-client/aspects-and-services',
        'java-client/navigation-and-views',
        'java-client/spider-templates',
        'java-client/look-and-feel',
        'java-client/playback-and-resolution',
        'java-client/musik-uri-scheme',
        'java-client/status-and-roadmap',
      ],
    },
  ],
};

module.exports = sidebars;
