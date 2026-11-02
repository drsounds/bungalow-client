function updateThemeColor() {
  const backgroundColor = getComputedStyle(document.documentElement)
    .getPropertyValue('--ifm-background-color')
    .trim();
  const themeColor = document.querySelector('meta[name="theme-color"]');

  if (backgroundColor && themeColor) {
    themeColor.setAttribute('content', backgroundColor);
  }
}

let themeObserver;

export function onRouteDidUpdate() {
  updateThemeColor();

  if (!themeObserver) {
    themeObserver = new MutationObserver(updateThemeColor);
    themeObserver.observe(document.documentElement, {
      attributes: true,
      attributeFilter: ['data-theme'],
    });
  }
}
