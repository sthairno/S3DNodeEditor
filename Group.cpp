#include "Group.hpp"

void NodeEditor::Group::layout(const Config& cfg)
{
	m_outRect = RectF(Rect);
	m_outRect.y -= cfg.font.height();
	m_outRect.h += cfg.font.height();

	m_titleRect = RectF(m_outRect.pos, m_outRect.w, cfg.font.height());

	m_titleFontRect = RectF(m_titleRect);
	m_titleFontRect.x += 5;
	m_titleFontRect.w -= 5 * 2;
}

void NodeEditor::Group::update(const Config& cfg, Input& input, Array<std::shared_ptr<Node>>& nodelist)
{
	layout(cfg);

	if (input.leftClicked(m_titleRect))
	{
		m_grab = true;
		m_grabTarget.clear();
		for (auto& node : nodelist)
		{
			if (Rect.contains(node->getRect()))
			{
				node->Selecting = true;
				m_grabTarget << node;
			}
		}
	}

	if (m_grab)
	{
		if (MouseL.pressed())
		{
			auto delta = Cursor::DeltaF();
			Rect.pos += delta;
			for (auto& node : m_grabTarget)
			{
				node->Location += delta;
			}
		}
		else
		{
			m_grab = false;
			m_grabTarget.clear();
		}
	}
}

void NodeEditor::Group::draw(const Config& cfg)
{
	layout(cfg);

	auto titleRoundRect = m_titleRect.rounded(5, 5, 0, 0);
	auto outRoundRect = m_outRect.rounded(5);
	outRoundRect.drawFrame();
	titleRoundRect.drawFrame();

	cfg.font(Name).draw(m_titleFontRect, Palette::White);
}
