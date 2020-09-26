#include<Siv3D.hpp>
#include"Input.hpp"
#include"NodeEditor.hpp"
#include"HamFramework.hpp"

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
				m_textbox = TextBox(cfg.font, { 0,0 }, 100, unspecified, Format(m_value));
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

		void childSerialize(JSONWriter& writer) const override
		{
			writer.write(m_value);
		}

		void childDeserialize(const JSONValue& json) override
		{
			m_value = json.get<decltype(m_value)>();
			if (m_textbox)
			{
				m_textbox->setText(Format(m_value));
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

namespace Input
{
	class KeyUpNode : public NodeEditor::INode
	{
	private:
		void childRun() override
		{
			setOutput(0, KeyUp.down());
			setOutput(1, KeyUp.pressed());
			setOutput(2, KeyUp.up());
		}

		void childDraw(const NodeEditor::Config& cfg) override
		{
			ChildSize = InputDeviceSymbol::GetSize(KeyUp, cfg.font, cfg.font.fontSize() * 2);
			InputDeviceSymbol::Draw(KeyUp, KeyUp.pressed(), { 0,0 }, cfg.font, cfg.font.fontSize() * 2);
		}
	public:
		KeyUpNode()
		{
			cfgOutputSockets({ {Type::getType<bool>(),U"down"},{Type::getType<bool>(),U"pressed"},{Type::getType<bool>(),U"up"} });

			ChildSize = SizeF(1, 0);

			Name = U"KeyUp";
		}
	};

	class KeyDownNode : public NodeEditor::INode
	{
	private:
		void childRun() override
		{
			setOutput(0, KeyDown.down());
			setOutput(1, KeyDown.pressed());
			setOutput(2, KeyDown.up());
		}

		void childDraw(const NodeEditor::Config& cfg) override
		{
			ChildSize = InputDeviceSymbol::GetSize(KeyDown, cfg.font, cfg.font.fontSize() * 2);
			InputDeviceSymbol::Draw(KeyDown, KeyDown.pressed(), { 0,0 }, cfg.font, cfg.font.fontSize() * 2);
		}
	public:
		KeyDownNode()
		{
			cfgOutputSockets({ {Type::getType<bool>(),U"down"},{Type::getType<bool>(),U"pressed"},{Type::getType<bool>(),U"up"} });

			ChildSize = SizeF(1, 0);

			Name = U"KeyDown";
		}
	};

	class KeyLeftNode : public NodeEditor::INode
	{
	private:
		void childRun() override
		{
			setOutput(0, KeyLeft.down());
			setOutput(1, KeyLeft.pressed());
			setOutput(2, KeyLeft.up());
		}

		void childDraw(const NodeEditor::Config& cfg) override
		{
			ChildSize = InputDeviceSymbol::GetSize(KeyLeft, cfg.font, cfg.font.fontSize() * 2);
			InputDeviceSymbol::Draw(KeyLeft, KeyLeft.pressed(), { 0,0 }, cfg.font, cfg.font.fontSize() * 2);
		}
	public:
		KeyLeftNode()
		{
			cfgOutputSockets({ {Type::getType<bool>(),U"down"},{Type::getType<bool>(),U"pressed"},{Type::getType<bool>(),U"up"} });

			ChildSize = SizeF(1, 0);

			Name = U"KeyLeft";
		}
	};

	class KeyRightNode : public NodeEditor::INode
	{
	private:
		void childRun() override
		{
			setOutput(0, KeyRight.down());
			setOutput(1, KeyRight.pressed());
			setOutput(2, KeyRight.up());
		}

		void childDraw(const NodeEditor::Config& cfg) override
		{
			ChildSize = InputDeviceSymbol::GetSize(KeyRight, cfg.font, cfg.font.fontSize() * 2);
			InputDeviceSymbol::Draw(KeyRight, KeyRight.pressed(), { 0,0 }, cfg.font, cfg.font.fontSize() * 2);
		}
	public:
		KeyRightNode()
		{
			cfgOutputSockets({ {Type::getType<bool>(),U"down"},{Type::getType<bool>(),U"pressed"},{Type::getType<bool>(),U"up"} });

			ChildSize = SizeF(1, 0);

			Name = U"KeyRight";
		}
	};
}

void RegisterNodes(NodeEditor::NodeEditor& editor, P2Body& player)
{
	editor.registerNodeType<UpdateFrameNode>(false);
	editor.registerNodeType<BranchNode>();
	editor.registerNodeType<Value::IntegerNode>();
	editor.registerNodeType<Input::KeyUpNode>();
	editor.registerNodeType<Input::KeyDownNode>();
	editor.registerNodeType<Input::KeyLeftNode>();
	editor.registerNodeType<Input::KeyRightNode>();

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
				if (auto node = editor.searchNode(U"UpdateFrameNode"))
				{
					updateNode = std::dynamic_pointer_cast<UpdateFrameNode>(*node);
				}
			}
		}
	}
}