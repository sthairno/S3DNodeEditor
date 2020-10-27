#pragma once
#include<Siv3D.hpp>
#include"Type.hpp"

namespace NodeEditor
{
	namespace detail
	{
		Image circleImage(const double& r, Color color, const Color& backColor = ColorF(0.0f, 0.0f));

		Image iconImage(const Icon& icon, const Color& color, const Color& backColor = ColorF(0.0f, 0.0f));
	}

	struct Config
	{
		ColorF Editor_BackCol = ColorF(0.3);

		float Node_WidthMin = 100;

		float Node_IOMargin = 10;

		float Node_TitleHeight = 20;

		float Node_RectR = 5;

		ColorF Node_TitleFontCol = Palette::Black;

		ColorF Node_ContentBackCol = ColorF(0.9);

		ColorF Node_ContentFontCol = Palette::Black;

		ColorF Node_SelectedFrameCol = Palette::Orange;

		float Group_RectR = 6;

		ColorF Group_BackCol = ColorF(0, 0.1);

		ColorF Group_TitleCol = ColorF(0, 0.2);

		ColorF Group_TitleFontCol = Palette::White;

		ColorF Group_FrameCol = Palette::White;

		ColorF RangeSelection_BackCol = ColorF(1, 0.1);

		ColorF RangeSelection_FrameCol = ColorF(1, 0.2);

		float ConnectorSize = 10;

		float BezierX = 50;

		Font font = Font(16);

		std::map<size_t, Texture> typeIconList = std::map<size_t, Texture>
		{
			{typeid(Image).hash_code(),Texture(detail::iconImage(Icon(0xf03e,10),Palette::Black))},
			{typeid(bool).hash_code(),Texture(detail::circleImage(10,Palette::Maroon))},
			{typeid(char).hash_code(),Texture(detail::circleImage(10,Palette::Darkgreen))},
			{typeid(int).hash_code(),Texture(detail::circleImage(10,Palette::Mediumaquamarine))},
			{typeid(float).hash_code(),Texture(detail::circleImage(10,Palette::Greenyellow))},
			{typeid(double).hash_code(),Texture(detail::circleImage(10,Palette::Greenyellow))},
			{typeid(String).hash_code(),Texture(detail::circleImage(10,Palette::Darkmagenta))}
		};

		const Optional<Texture> getTypeIcon(const Type& type) const
		{
			const auto itr = typeIconList.find(type.TypeInfo().hash_code());
			if (itr == typeIconList.end())
			{
				return none;
			}
			else
			{
				return itr->second;
			}
		}
	};

	enum class IOType
	{
		Input, Output
	};

	class ISerializable
	{
	public:

		virtual void serialize(JSONWriter&) const = 0;

		virtual void deserialize(const JSONValue&) = 0;
	};
}
