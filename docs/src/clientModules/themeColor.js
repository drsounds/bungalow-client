function updateThemeColor() {
  const backgroundColor = getComputedStyle(document.documentElement)
    .getPropertyValue('--ifm-background-color')
    .trim();
  const themeColor = document.querySelector('meta[name="theme-color"]');

  if (backgroundColor && themeColor && themeColor.getAttribute('content') !== backgroundColor) {
    themeColor.setAttribute('content', backgroundColor);
  }
}

if (typeof document !== 'undefined') {
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', updateThemeColor, {once: true});
  } else {
    updateThemeColor();
  }

  const themeObserver = new MutationObserver(updateThemeColor);
  themeObserver.observe(document.documentElement, {
    attributes: true,
    attributeFilter: ['data-theme'],
  });
  themeObserver.observe(document.head, {
    attributes: true,
    attributeFilter: ['content', 'name'],
    childList: true,
    subtree: true,
  });
}

export function onRouteDidUpdate() {
  updateThemeColor();
}
