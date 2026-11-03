import React from 'react';
import ComponentCreator from '@docusaurus/ComponentCreator';

export default [
  {
    path: '/bungalow-client/',
    component: ComponentCreator('/bungalow-client/', '98e'),
    routes: [
      {
        path: '/bungalow-client/',
        component: ComponentCreator('/bungalow-client/', 'ece'),
        routes: [
          {
            path: '/bungalow-client/',
            component: ComponentCreator('/bungalow-client/', '5af'),
            routes: [
              {
                path: '/bungalow-client/apps/bundles',
                component: ComponentCreator('/bungalow-client/apps/bundles', '3b3'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/apps/html5-apps',
                component: ComponentCreator('/bungalow-client/apps/html5-apps', 'ab1'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/apps/native-apps',
                component: ComponentCreator('/bungalow-client/apps/native-apps', '5f9'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/apps/sites-and-bookmarks',
                component: ComponentCreator('/bungalow-client/apps/sites-and-bookmarks', '82b'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/architecture/aspects',
                component: ComponentCreator('/bungalow-client/architecture/aspects', 'e34'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/architecture/data',
                component: ComponentCreator('/bungalow-client/architecture/data', '16d'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/architecture/features-and-concepts',
                component: ComponentCreator('/bungalow-client/architecture/features-and-concepts', 'c24'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/architecture/navigation',
                component: ComponentCreator('/bungalow-client/architecture/navigation', 'f6e'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/architecture/overview',
                component: ComponentCreator('/bungalow-client/architecture/overview', 'a8e'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/architecture/playback',
                component: ComponentCreator('/bungalow-client/architecture/playback', '265'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/architecture/search',
                component: ComponentCreator('/bungalow-client/architecture/search', '9ef'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/background',
                component: ComponentCreator('/bungalow-client/background', '7ec'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/getting-started/building',
                component: ComponentCreator('/bungalow-client/getting-started/building', '16b'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/getting-started/project-layout',
                component: ComponentCreator('/bungalow-client/getting-started/project-layout', '6ef'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/java-client/',
                component: ComponentCreator('/bungalow-client/java-client/', 'b9e'),
                exact: true,
                sidebar: "javaClient"
              },
              {
                path: '/bungalow-client/java-client/architecture',
                component: ComponentCreator('/bungalow-client/java-client/architecture', '370'),
                exact: true,
                sidebar: "javaClient"
              },
              {
                path: '/bungalow-client/java-client/aspects-and-services',
                component: ComponentCreator('/bungalow-client/java-client/aspects-and-services', '818'),
                exact: true,
                sidebar: "javaClient"
              },
              {
                path: '/bungalow-client/java-client/look-and-feel',
                component: ComponentCreator('/bungalow-client/java-client/look-and-feel', '63c'),
                exact: true,
                sidebar: "javaClient"
              },
              {
                path: '/bungalow-client/java-client/musik-uri-scheme',
                component: ComponentCreator('/bungalow-client/java-client/musik-uri-scheme', '037'),
                exact: true,
                sidebar: "javaClient"
              },
              {
                path: '/bungalow-client/java-client/navigation-and-views',
                component: ComponentCreator('/bungalow-client/java-client/navigation-and-views', 'a27'),
                exact: true,
                sidebar: "javaClient"
              },
              {
                path: '/bungalow-client/java-client/playback-and-resolution',
                component: ComponentCreator('/bungalow-client/java-client/playback-and-resolution', '440'),
                exact: true,
                sidebar: "javaClient"
              },
              {
                path: '/bungalow-client/java-client/plugin-system',
                component: ComponentCreator('/bungalow-client/java-client/plugin-system', '9c8'),
                exact: true,
                sidebar: "javaClient"
              },
              {
                path: '/bungalow-client/java-client/spider-templates',
                component: ComponentCreator('/bungalow-client/java-client/spider-templates', '4d9'),
                exact: true,
                sidebar: "javaClient"
              },
              {
                path: '/bungalow-client/java-client/status-and-roadmap',
                component: ComponentCreator('/bungalow-client/java-client/status-and-roadmap', '9e3'),
                exact: true,
                sidebar: "javaClient"
              },
              {
                path: '/bungalow-client/reference/settings-and-files',
                component: ComponentCreator('/bungalow-client/reference/settings-and-files', '735'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/reference/uri-schemes',
                component: ComponentCreator('/bungalow-client/reference/uri-schemes', 'bbd'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/roadmap',
                component: ComponentCreator('/bungalow-client/roadmap', '2bc'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/space/backends',
                component: ComponentCreator('/bungalow-client/space/backends', '259'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/space/elements-and-xul',
                component: ComponentCreator('/bungalow-client/space/elements-and-xul', '5b6'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/space/overview',
                component: ComponentCreator('/bungalow-client/space/overview', 'eed'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/space/skins',
                component: ComponentCreator('/bungalow-client/space/skins', 'b59'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/space/templates',
                component: ComponentCreator('/bungalow-client/space/templates', '23c'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/space/uri-store',
                component: ComponentCreator('/bungalow-client/space/uri-store', '9c5'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/specs/',
                component: ComponentCreator('/bungalow-client/specs/', '96c'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/specs/rfc-0001',
                component: ComponentCreator('/bungalow-client/specs/rfc-0001', 'b2b'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/specs/rfc-0002',
                component: ComponentCreator('/bungalow-client/specs/rfc-0002', '38c'),
                exact: true,
                sidebar: "docs"
              },
              {
                path: '/bungalow-client/',
                component: ComponentCreator('/bungalow-client/', '1ad'),
                exact: true,
                sidebar: "docs"
              }
            ]
          }
        ]
      }
    ]
  },
  {
    path: '*',
    component: ComponentCreator('*'),
  },
];
