#pragma once
#include"INode.hpp"
#include"Input.hpp"
#include<Siv3D.hpp>

namespace NodeEditor
{
	class Group
	{
	private:

		bool m_grab = false;

		RectF m_outRect;

		RectF m_titleRect;

		RectF m_titleFontRect;

		Array<std::shared_ptr<INode>> m_grabTarget;

		void layout(const Config& cfg);

	public:

		RectF Rect;

		String Name;

		void update(const Config& cfg, Input& input, Array<std::shared_ptr<INode>>& nodelist);

		void draw(const Config& cfg);
	};
}
