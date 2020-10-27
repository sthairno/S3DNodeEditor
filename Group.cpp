#include "Group.hpp"

void NodeEditor::Group::layout(const Config& cfg)
{
	m_outRect = RectF(Rect);
	m_outRect.y -= cfg.font.height();
	m_outRect.h += cfg.font.height();

	m_titleRect = RectF(m_outRect.pos, m_outRect.w, cfg.font.height());

	m_titleFontRect = RectF(m_titleRect);
	m_titleFontRect.x += cfg.Group_RectR;
	m_titleFontRect.w -= cfg.Group_RectR * 2;
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

	auto titleRoundRect = m_titleRect.rounded(cfg.Group_RectR, cfg.Group_RectR, 0, 0);
	auto outRoundRect = m_outRect.rounded(cfg.Group_RectR);
	outRoundRect.draw(cfg.Group_BackCol).drawFrame(0, 1, cfg.Group_FrameCol);
	titleRoundRect.draw(cfg.Group_TitleCol);
	RectF(m_titleRect.bl(), m_titleRect.w, 1).draw(cfg.Group_FrameCol);

	cfg.font(Name).draw(m_titleFontRect, cfg.Group_TitleFontCol);
}
