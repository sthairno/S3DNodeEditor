#include<Siv3D.hpp>
#include<any>
#include<typeinfo>
#include"Input.h"
#include"NodeEditor.h"

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
			cfgInputSockets({ });
			cfgOutputSockets({ {Type::getType<int>(),U"Val"} });

			ChildSize = SizeF(1, 0);

			Name = U"Int32";
		}
	};

	class RealNode : public NodeEditor::INode
	{
	private:

		void childRun() override
		{
			setOutput(0, Value);
		}

		void childDraw(const NodeEditor::Config& cfg) override
		{
			if (KeyW.down())Value++;
			if (KeyS.down())Value--;
			cfg.font(Value).draw(0, 0, Palette::Black);
		}

	public:

		double Value = Random(-100, 100);

		RealNode()
		{
			cfgInputSockets({ });
			cfgOutputSockets({ {Type::getType<double>(),U"Val"} });

			ChildSize = SizeF(20, 20);

			Name = U"double";
		}
	};
}

namespace Calc
{
	class IncrementNode : public NodeEditor::INode
	{
	private:

		void childRun() override
		{
			auto val = getInput<int>(0);

			setOutput(0, val + 1);
		}

	public:

		IncrementNode()
		{
			cfgInputSockets({ {Type::getType<int>(),U"A"} });
			cfgOutputSockets({ {Type::getType<int>(),U"A+1"} });

			ChildSize = SizeF(0, 0);

			Name = U"Inc";
		}
	};

	class AddNode : public NodeEditor::INode
	{
	private:

		void childRun() override
		{
			auto valA = getInput<int>(0);
			auto valB = getInput<int>(1);

			setOutput(0, valA + valB);
		}

	public:

		AddNode()
		{
			cfgInputSockets({ {Type::getType<int>(),U"A"},{Type::getType<int>(),U"B"} });
			cfgOutputSockets({ {Type::getType<int>(),U"A+B"} });

			ChildSize = SizeF(0, 0);

			Name = U"Add";
		}
	};

	class IsEqualNode : public NodeEditor::INode
	{
	private:

		void childRun() override
		{
			auto a = getInput<int>(0);
			auto b = getInput<int>(1);

			setOutput(0, a == b);
		}

	public:

		IsEqualNode()
		{
			cfgInputSockets({ {Type::getType<int>(),U"A"},{Type::getType<int>(),U"B"} });
			cfgOutputSockets({ {Type::getType<bool>(),U"A==B"} });
			cfgPrevExecSocket({});
			cfgNextExecSocket({});

			Name = U"Equal";
		}
	};
}

class PreviewNode : public NodeEditor::INode
{
private:

	Optional<String> result;

	void childRun() override
	{
		try
		{
			result = Format(getInput<int>(0));
		}
		catch (std::exception ex)
		{
			result = Unicode::FromUTF8(ex.what());
		}
	}

	void childDraw(const NodeEditor::Config& cfg) override
	{
		String text;

		text = Format(result);

		cfg.font(text).draw(0, 0, Palette::Black);
	}

public:

	PreviewNode()
	{
		cfgInputSockets({ {Type::getType<int>(),U"Val"} });
		cfgNextExecSocket({ U"" });
		cfgPrevExecSocket({ U"" });

		ChildSize = SizeF(20, 20);

		Name = U"Preview";
	}
};

template<typename InputType, typename OutputType>
class CastNode : public NodeEditor::INode
{
private:

	void childRun() override
	{
		auto val = getInput<InputType>(0);

		setOutput(0, static_cast<OutputType>(val));
	}

public:

	CastNode()
	{
		Type inputType = Type::getType<InputType>();
		Type outputType = Type::getType<OutputType>();
		cfgInputSockets({ {inputType,inputType.name()} });
		cfgOutputSockets({ {outputType,outputType.name()} });

		ChildSize = SizeF(0, 0);

		Name = U"Cast";
	}
};

class UpdateFrameNode : public NodeEditor::INode
{
public:

	UpdateFrameNode()
	{
		cfgNextExecSocket({ U"" });

		ChildSize = SizeF(0, 0);

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
		cfgOutputSockets({ });
		cfgPrevExecSocket({ U"" });
		cfgNextExecSocket({ U"True",U"False" });

		ChildSize = SizeF(0, 0);

		Name = U"Branch";
	}
};

class PrintNode : public NodeEditor::INode
{
private:

	void childRun() override
	{
		Print << getInput<String>(0);
	}

public:

	PrintNode()
	{
		cfgInputSockets({ {Type::getType<String>(),U"Text"} });
		cfgOutputSockets({ });
		cfgPrevExecSocket({ U"" });
		cfgNextExecSocket({ U"" });

		Name = U"Print";
	}
};

void Main()
{
	Window::Resize(1280, 800);

	Vec2 playerPos(0, 0);

	const int gripHeight = 5;
	bool gripGrab = false;

	int editorHeight = 300;

	Size nodeEditorSize(Scene::Width(), editorHeight);

	NodeEditor::NodeEditor editor(nodeEditorSize);
	
	editor.registerNodeType<Value::IntegerNode>();
	editor.registerNodeType<Value::RealNode>();

	editor.registerNodeType<Calc::IncrementNode>();
	editor.registerNodeType<Calc::AddNode>();
	editor.registerNodeType<Calc::IsEqualNode>();

	editor.registerNodeType<CastNode<int, double>>();
	editor.registerNodeType<CastNode<double, int>>();

	editor.registerNodeType<PreviewNode>();
	editor.registerNodeType<BranchNode>();
	editor.registerNodeType<PrintNode>();

	editor.registerNodeFunction<void(Point)>(U"Player::Move", { U"vec" }, [&](Point vec)
		{
			playerPos += vec;
		});

	editor.registerNodeFunction<Point(int, int)>(U"Point::Point(int,int)", { U"Point",U"x",U"y" }, [](int x, int y)
		{
			return Point(x, y);
		});

	editor.registerNodeFunction<bool()>(U"KeyUp.pressed()", { U"" }, []()
		{
			return KeyUp.pressed();
		});

	editor.registerNodeFunction<bool()>(U"KeyDown.pressed()", { U"" }, []()
		{
			return KeyDown.pressed();
		});

	editor.registerNodeFunction<bool()>(U"KeyLeft.pressed()", { U"" }, []()
		{
			return KeyLeft.pressed();
		});

	editor.registerNodeFunction<bool()>(U"KeyRight.pressed()", { U"" }, []()
		{
			return KeyRight.pressed();
		});

	editor.registerNodeFunction<void(int, double)>(U"TestFunc", { U"a",U"b" }, [](int a, double b)
		{
			Print << U"{},{}"_fmt(a, b);
		});

	auto updateNode = std::make_shared<UpdateFrameNode>();
	editor.addNode(updateNode);

	while (System::Update())
	{
		auto gripRect = RectF(0, Scene::Height() - editorHeight - gripHeight, Scene::Width(), gripHeight);

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

		updateNode->run();

		Circle(playerPos, 10).draw(Palette::Red);
	}
}