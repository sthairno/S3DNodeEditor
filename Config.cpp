#include "Config.h"

Image NodeEditor::detail::circleImage(const double& r, Color color, const Color& backColor)
{
	const auto size = static_cast<size_t>(Ceil(r * 2));
	Image img(size, size, backColor);
	Circle(r - 0.5, r - 0.5, r).overwrite(img, color, false);
	return img;
}

Image NodeEditor::detail::iconImage(const Icon& icon, const Color& color, const Color& backColor)
{
	Image iconImg = Icon::CreateImage(icon.code, icon.size);
	Image img = Image(iconImg.size(), backColor);
	iconImg.overwrite(img, { 0,0 }, color);
	return img;
}
