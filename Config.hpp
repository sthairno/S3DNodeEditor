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
		float WidthMin = 100;

		float IOMargin = 10;

		float TitleHeight = 20;

		float RectR = 5;

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
