#include<Siv3D.hpp>
#include"Input.hpp"
#include"NodeEditor.hpp"

class UpdateFrameNode : public NodeEditor::INode
{
public:

	UpdateFrameNode()
	{
		cfgNextExecSocket({ U"" });

		Name = U"Update";
	}
};

class BranchNode : public NodeEditor::INode
{
private:

	void childRun() override
	{
		if (getInput<bool>(0))
		{
			NextExecIdx = 0;
		}
		else
		{
			NextExecIdx = 1;
		}
	}

public:

	BranchNode()
	{
		cfgInputSockets({ {Type::getType<bool>(),U"Cond"} });
		cfgPrevExecSocket({ U"" });
		cfgNextExecSocket({ U"True",U"False" });

		Name = U"Branch";
	}
};

namespace Value
{
	class IntegerNode : public NodeEditor::INode
	{
	private:

		Optional<TextBox> m_textbox;

		int m_value = 0;

		void childRun() override
		{
			setOutput(0, m_value);
		}

		void childUpdate(const NodeEditor::Config& cfg, NodeEditor::Input& input) override
		{
			bool init = !m_textbox;

			if (init)
			{
				m_textbox = TextBox(cfg.font, { 0,0 }, 100, unspecified, U"0");
			}

			switch (m_textbox->update(!input.getProc() && MouseL.down()))
			{
			case TextBox::State::Active:
				if (MouseL.down())
				{
					input.setProc();
				}
				break;
			case TextBox::State::Enter:
			case TextBox::State::Tab:
			case TextBox::State::ShiftTab:
				auto result = ParseIntOpt<int>(m_textbox->getText());
				if (result)
				{
					m_value = result.value();
				}
				else
				{
					m_textbox->setText(U"0");
				}
				break;
			}

			if (init)
			{
				ChildSize = m_textbox->getRect().size;
			}
		}

		void childDraw(const NodeEditor::Config&) override
		{
			if (m_textbox->isActive())
			{
				m_textbox->drawOverlay();
			}
			else
			{
				m_textbox->draw();
			}
		}

	public:

		IntegerNode()
		{
			cfgOutputSockets({ {Type::getType<int>(),U"Val"} });

			ChildSize = SizeF(1, 0);

			Name = U"Int32";
		}
	};
}

void RegisterNodes(NodeEditor::NodeEditor& editor, P2Body& player)
{
	editor.registerNodeType<UpdateFrameNode>();
	editor.registerNodeType<BranchNode>();
	editor.registerNodeType<Value::IntegerNode>();

	editor.registerNodeFunction<bool()>(U"Input::KeyUp.pressed", { U"" }, []()
		{
			return KeyUp.pressed();
		});
	editor.registerNodeFunction<bool()>(U"Input::KeyUp.down", { U"" }, []()
		{
			return KeyUp.down();
		});
	editor.registerNodeFunction<bool()>(U"Input::KeyDown.pressed", { U"" }, []()
		{
			return KeyDown.pressed();
		});
	editor.registerNodeFunction<bool()>(U"Input::KeyDown.down", { U"" }, []()
		{
			return KeyDown.down();
		});
	editor.registerNodeFunction<bool()>(U"Input::KeyLeft.pressed", { U"" }, []()
		{
			return KeyLeft.pressed();
		});
	editor.registerNodeFunction<bool()>(U"Input::KeyLeft.down", { U"" }, []()
		{
			return KeyLeft.down();
		});
	editor.registerNodeFunction<bool()>(U"Input::KeyRight.pressed", { U"" }, []()
		{
			return KeyRight.pressed();
		});
	editor.registerNodeFunction<bool()>(U"Input::KeyRight.down", { U"" }, []()
		{
			return KeyRight.down();
		});

	editor.registerNodeFunction<void(Point)>(U"Player::AddForce", { U"Point" }, [&](Point point)
		{
			return player.applyForce(point);
		});

	editor.registerNodeFunction<Point(int, int)>(U"Point::Point(int,int)", { U"Point",U"x",U"y" }, [](int x, int y)
		{
			return Point(x, y);
		});
}

void Main()
{
	Window::Resize(1280, 800);

	const int gripHeight = 5;
	bool gripGrab = false;

	int editorHeight = 300;
	bool editorVisible = true;

	Size nodeEditorSize(Scene::Width(), editorHeight);

	NodeEditor::NodeEditor editor(nodeEditorSize);

	// 2D 物理演算
	Camera2D camera(Vec2(0, 0), 20.0, Camera2DParameters::NoControl());
	P2World world(9.8);
	const P2Body line = world.createStaticLine(Vec2(0, 0), Line(-10, 0, 60, 0));
	const P2Body block = world.createStaticRect({ 20,-1 }, SizeF(2, 2));
	P2Body plus = world.createStaticPolygon({ 40,-6 }, Shape2D::Plus(5, 0.4).asPolygon());

	P2Body player = world.createCircle({ 0,-5 }, 1);

	// ノード登録
	RegisterNodes(editor, player);

	auto updateNode = *editor.addNode<UpdateFrameNode>();

	while (System::Update())
	{
		// 2D カメラを更新
		camera.update();
		{
			const auto t = camera.createTransformer();
			line.draw(Palette::Skyblue);
			block.draw(Palette::White);
			plus.draw(Palette::White);
			player.draw(Palette::Red);
		}

		auto gripRect = RectF(0, Scene::Height() - editorHeight - gripHeight, Scene::Width(), gripHeight);

		auto visibleBtn = RectF(Arg::bottomRight = editorVisible ? gripRect.tr() : Scene::Size(), 40, 20);
		if (visibleBtn.leftClicked())
		{
			editorVisible = !editorVisible;
		}
		visibleBtn.draw();
		if (editorVisible)
		{
			if (gripRect.mouseOver())
			{
				Cursor::RequestStyle(CursorStyle::ResizeUpDown);
				if (MouseL.down())
				{
					gripGrab = true;
				}
			}
			if (gripGrab)
			{
				Cursor::RequestStyle(CursorStyle::ResizeUpDown);
				editorHeight -= Cursor::Delta().y;
				editorHeight = Clamp(editorHeight, 100, Scene::Height() - gripHeight);
				if (!MouseL.pressed())
				{
					nodeEditorSize.y = editorHeight;
					editor.resize(nodeEditorSize);
					gripGrab = false;
				}
			}

			editor.draw({ 0,Scene::Height() - editorHeight });
			gripRect.draw();
		}

		updateNode->run();
		plus.setAngle(Periodic::Sawtooth0_1(2s) * Math::TwoPi);
		camera.setTargetCenter(player.getPos());

		world.update();

		if (SimpleGUI::Button(U"Save", { 10,10 }))
		{
			if (auto path = Dialog::SaveFile({ FileFilter::JSON() }, U"node.json"))
			{
				TextWriter(*path).write(editor.save());
			}
		}

		if (SimpleGUI::Button(U"Load", { 10,50 }))
		{
			if (auto path = Dialog::OpenFile({ FileFilter::JSON() }, U"node.json"))
			{
				editor.load(JSONReader(*path));
				//UpdateFrameNodeのインスタンスを検索
				updateNode = std::dynamic_pointer_cast<UpdateFrameNode>(*editor.searchNode(U"UpdateFrameNode"));
			}
		}
	}
}