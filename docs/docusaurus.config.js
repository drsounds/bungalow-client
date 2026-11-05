// @ts-check
const {themes: prismThemes} = require('prism-react-renderer');

/** @type {import('@docusaurus/types').Config} */
const config = {
  title: 'Bungalow',
  tagline: 'An open, plugin operating system for music, built on the Space UI framework',
  favicon: 'img/logo.svg',

  url: 'https://drsounds.github.io',
  baseUrl: '/bungalow-client/',
  organizationName: 'drsounds',
  projectName: 'bungalow-client',
  clientModules: [require.resolve('./src/clientModules/themeColor.js')],

  onBrokenLinks: 'throw',

  // Plain .md files are parsed as CommonMark, so C++ signatures such as
  // AspectManager<T> or { … } in prose need no MDX escaping.
  markdown: {
    format: 'detect',
    hooks: {
      onBrokenMarkdownLinks: 'throw',
    },
  },

  i18n: {
    defaultLocale: 'en',
    locales: ['en'],
  },

  presets: [
    [
      'classic',
      /** @type {import('@docusaurus/preset-classic').Options} */
      ({
        docs: {
          routeBasePath: 'docs',
          sidebarPath: require.resolve('./sidebars.js'),
          editUrl: 'https://github.com/drsounds/bungalow-client/tree/main/docs/',
        },
        blog: false,
        theme: {
          customCss: require.resolve('./src/css/custom.css'),
        },
      }),
    ],
  ],

  themeConfig:
    /** @type {import('@docusaurus/preset-classic').ThemeConfig} */
    ({
      colorMode: {
        defaultMode: 'dark',
        respectPrefersColorScheme: false,
      },
      metadata: [{name: 'theme-color', content: '#211e1a'}],
      navbar: {
        title: 'Bungalow',
        logo: {
          alt: 'Bungalow',
          src: 'img/logo.svg',
        },
        items: [
          {type: 'docSidebar', sidebarId: 'docs', position: 'left', label: 'Docs'},
          {to: '/docs/specs/', label: 'Specifications', position: 'left'},
          {to: '/docs/java-client/', label: 'Java client', position: 'left'},
          {
            href: 'https://github.com/drsounds/bungalow-client',
            label: 'GitHub',
            position: 'right',
          },
        ],
      },
      footer: {
        style: 'dark',
        links: [
          {
            title: 'Docs',
            items: [
              {label: 'Introduction', to: '/docs/'},
              {label: 'Building', to: '/docs/getting-started/building'},
              {label: 'Architecture', to: '/docs/architecture/overview'},
              {label: 'Specifications', to: '/docs/specs/'},
            ],
          },
          {
            title: 'Source',
            items: [
              {label: 'C++ client', href: 'https://github.com/drsounds/bungalow-client'},
              {label: 'Java client', href: 'https://github.com/drsounds/bungalow'},
            ],
          },
        ],
        copyright: `Bungalow is MIT licensed. Built with Docusaurus.`,
      },
      prism: {
        theme: prismThemes.github,
        darkTheme: prismThemes.dracula,
        additionalLanguages: ['cpp', 'java', 'lua', 'json', 'bash', 'powershell', 'cmake'],
      },
    }),
};

module.exports = config;
