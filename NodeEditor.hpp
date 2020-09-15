#pragma once
#include<Siv3D.hpp>
#include<regex>
#include"Input.hpp"
#include"Config.hpp"
#include"INode.hpp"
#include"NodeSocket.hpp"

namespace NodeEditor
{
	namespace detail
	{
		Array<String> split(const String& str, const String& key)
		{
			Array<String> result;
			size_t start = 0, end = 0;
			while ((end = str.indexOf(key, start)) != String::npos)
			{
				result << str.substr(start, end - start);
				start = end + key.length();
			}
			result << str.substr(start, str.length());
			return result;
		}

		class INodeGenerator
		{
		private:

			using GeneratorType = std::function<std::shared_ptr<INode>(void)>;

			std::regex prefixRegex = std::regex("^(?:class |struct )?(.*)$");

			template<class SubType>
			GeneratorType createGenerator(const String& className)
			{
				return [=]()
				{
					auto inode = std::make_shared<SubType>();
					inode->Class = className;
					return inode;
				};
			}

			template<class FuncType>
			GeneratorType createFuncGenerator(const String& className, const String& name, const Array<String>& argNames, const std::function<FuncType>& function)
			{
				return [=]()
				{
					auto inode = std::make_shared<detail::FunctionNode<FuncType>>(name, argNames, function);
					inode->Class = className;
					return inode;
				};
			}

			Array<String> parseNames(const String& name)
			{
				std::cmatch match;
				std::string stdstring = name.toUTF8();
				std::regex_match(stdstring.c_str(), match, prefixRegex);

				return detail::split(Unicode::FromUTF8(match[1].str()), U"::");
			}

		public:

			struct INodeClass
			{
				bool isFunction;
				GeneratorType generator;
			};

			struct Group
			{
				std::unordered_map<String, INodeClass> classes;
				std::unordered_map<String, Group> namespaces;
				String ToString(const String& prefix = U"")
				{
					String str;
					for (auto& keyval : classes)
					{
						str += prefix + keyval.first + U"\n";
					}
					for (auto& keyval : namespaces)
					{
						str += keyval.second.ToString(prefix + keyval.first + U"::");
					}
					return str;
				}
			};

			Group global;

			template<class SubType>
			void registerType()
			{
				Type type = Type::getType<SubType>();

				auto names = parseNames(Unicode::FromUTF8(typeid(SubType).name()));

				auto targetNamespace = std::ref(global);
				for (size_t i = 0; i < names.size() - 1; i++)
				{
					targetNamespace = targetNamespace.get().namespaces[names[i]];
				}
				targetNamespace.get().classes.emplace(names[names.size() - 1], INodeClass{ false,createGenerator<SubType>(names.join(U"::",U"",U"")) });
			}

			template<class FuncType>
			void registerFunction(const String& name, const Array<String>& argNames, const std::function<FuncType>& function)
			{
				auto names = parseNames(name);

				auto targetNamespace = std::ref(global);
				for (size_t i = 0; i < names.size() - 1; i++)
				{
					targetNamespace = targetNamespace.get().namespaces[names[i]];
				}
				targetNamespace.get().classes.emplace(names[names.size() - 1], INodeClass{ true,createFuncGenerator<FuncType>(names.join(U"::",U"",U""),names[names.size() - 1],argNames,function) });
			}

			Optional<std::shared_ptr<INode>> getINode(const Type& type)
			{
				return getINode(parseNames(type.name()));
			}

			Optional<std::shared_ptr<INode>> getINode(const String& names)
			{
				return getINode(parseNames(names));
			}

			Optional<std::shared_ptr<INode>> getINode(const Array<String>& names)
			{
				auto targetNamespace = std::ref(global);
				for (size_t i = 0; i < names.size() - 1; i++)
				{
					targetNamespace = targetNamespace.get().namespaces[names[i]];
				}
				auto result = targetNamespace.get().classes.find(names[names.size() - 1]);
				if (result == targetNamespace.get().classes.end())
				{
					return none;
				}
				else
				{
					return result->second.generator();
				}
			}
		};

		class NodeListWindow
		{
		private:

			const int width = 180;
			const int lineCnt = 10;

			bool m_visible = false;

			const INodeGenerator& m_generator;

			const Texture m_nsTexture = Texture(U"icons/namespace.png");

			const Texture m_classTexture = Texture(U"icons/class.png");

			const Texture m_functionTexture = Texture(U"icons/function.png");

			const Texture m_anglerightTexture = Texture(Icon(0xf105, 16));

			std::pair<String, INodeGenerator::Group> m_currentNs;

		public:

			Vec2 m_location;

			NodeListWindow(const INodeGenerator& generator)
				:m_generator(generator)
			{

			}

			void show(const Vec2& pos)
			{
				m_currentNs = { U"global",m_generator.global };
				m_location = pos;
				m_visible = true;
			}

			void hide()
			{
				m_visible = false;
			}

			std::shared_ptr<INode> update(const Config& cfg, Input& input)
			{
				std::shared_ptr<INode> result;

				if (m_visible)
				{
					const RectF rect(m_location, width, cfg.font.height() * (lineCnt + 1));
					const RectF contentRect(m_location.x, m_location.y + cfg.font.height(), width, cfg.font.height() * lineCnt);
					Vec2 fontPos = contentRect.pos;

					for (auto& keyval : m_currentNs.second.namespaces)
					{
						if (input.leftClicked(RectF(fontPos, width, cfg.font.height())))
						{
							m_currentNs = keyval;
							goto exitInput;
						}
						fontPos.y += cfg.font.height();
					}
					for (auto& keyval : m_currentNs.second.classes)
					{
						if (input.leftClicked(RectF(fontPos, width, cfg.font.height())))
						{
							result = keyval.second.generator();
							hide();
							goto exitInput;
						}
						fontPos.y += cfg.font.height();
					}

					if (rect.mouseOver())
					{
						input.setProc();
					}

					if (KeyBackspace.down())
					{
						hide();
					}

				exitInput:;
				}

				return result;
			}

			void draw(const Config& cfg)
			{
				if (m_visible)
				{
					const RectF rect(m_location, width, cfg.font.height() * (lineCnt + 1));
					const RectF titleRect(m_location, width, cfg.font.height());
					const RectF contentRect(m_location.x, m_location.y + cfg.font.height(), width, cfg.font.height() * lineCnt);

					rect.draw(ColorF(0.9));
					titleRect.draw(ColorF(0.7));
					cfg.font(m_currentNs.first).drawAt(titleRect.center(), Palette::Black);

					Vec2 fontPos = contentRect.pos;
					for (auto& keyval : m_currentNs.second.namespaces)
					{
						RectF btnRect(fontPos, width, cfg.font.height());
						cfg.font(keyval.first).draw(
							RectF(btnRect.x + m_nsTexture.width(), btnRect.y, btnRect.w - m_nsTexture.width() - m_anglerightTexture.width(), btnRect.h)
							, Palette::Black);
						m_nsTexture.draw(Arg::leftCenter = btnRect.leftCenter());
						m_anglerightTexture.draw(Arg::rightCenter = btnRect.rightCenter(), Palette::Black);
						fontPos.y += cfg.font.height();
					}
					for (auto& keyval : m_currentNs.second.classes)
					{
						RectF btnRect(fontPos, width, cfg.font.height());
						auto tex = keyval.second.isFunction ? m_functionTexture : m_classTexture;
						cfg.font(keyval.first).draw(
							RectF(btnRect.x + tex.width(), btnRect.y, btnRect.w - tex.width(), btnRect.h)
							, Palette::Black);
						tex.draw(Arg::leftCenter = btnRect.leftCenter());
						fontPos.y += cfg.font.height();
					}
				}
			}
		};

		class EditorCamera2D : public BasicCamera2D
		{
		protected:

			double m_targetScale = BasicCamera2D::m_scale;
			double m_scaleChangeVelocity = 0.0;

			Vec2 m_targetCenter = BasicCamera2D::m_center;
			Vec2 m_positionChangeVelocity = Vec2::Zero();

			bool m_grab;
			Optional<std::pair<Point, Vec2>> m_pointedScale;

			Camera2DParameters m_setting;

			Mat3x2 m_defaultGraphLocalTransform;

			Mat3x2 m_defaultCursorLocalTransform;

			Mat3x2 m_defaultGraphCameraTransform;

			Mat3x2 m_defaultCursorCameraTransform;

			void updateWheel(const SizeF& sceneSize)
			{
				const double wheel = Mouse::Wheel();

				if (wheel == 0.0)
				{
					return;
				}

				m_positionChangeVelocity = Vec2::Zero();

				if (wheel < 0.0)
				{
					m_targetScale *= m_setting.wheelScaleFactor;
				}
				else
				{
					m_targetScale /= m_setting.wheelScaleFactor;
				}

				m_targetScale = Clamp(m_targetScale, m_setting.minScale, m_setting.maxScale);

				const Point cursorPos = Cursor::Pos();
				const Vec2 point = m_center + (cursorPos - (sceneSize * 0.5)) / m_scale;
				m_pointedScale.emplace(cursorPos, point);
			}

			void updateMouse(Input& input)
			{
				if (input.down(MouseL))
				{
					m_grab = true;
					m_pointedScale.reset();
				}
				else if (m_grab)
				{
					m_targetCenter -= Cursor::DeltaF() / m_scale;

					if (MouseL.up())
					{
						m_grab = false;
					}
				}
			}

		public:

			EditorCamera2D(const Vec2& center, double scale = 1.0) noexcept
				: BasicCamera2D(center, scale)
				, m_setting(Camera2DParameters::Default())
			{
				m_setting.positionSmoothTime = 0.03;
			}

			~EditorCamera2D() = default;

			void setDefaultTransform()
			{
				m_defaultGraphLocalTransform = Graphics2D::GetLocalTransform();
				m_defaultCursorLocalTransform = Cursor::GetLocalTransform();
				m_defaultGraphCameraTransform = Graphics2D::GetCameraTransform();
				m_defaultCursorCameraTransform = Cursor::GetCameraTransform();
			}

			void setParameters(const Camera2DParameters& setting) noexcept
			{
				m_setting = setting;
			}

			[[nodiscard]] const Camera2DParameters& getParameters() const noexcept
			{
				return m_setting;
			}

			void setTargetCenter(const Vec2& targetCenter) noexcept
			{
				m_grab = false;
				m_pointedScale.reset();
				m_targetCenter = targetCenter;
			}

			void setTargetScale(double targetScale) noexcept
			{
				m_grab = false;
				m_pointedScale.reset();
				m_targetScale = targetScale;
			}

			void jumpTo(const Vec2& center, double scale) noexcept
			{
				m_grab = false;
				m_pointedScale.reset();
				m_targetCenter = m_center = center;
				m_targetScale = m_scale = scale;
				m_positionChangeVelocity = Vec2::Zero();
				m_scaleChangeVelocity = 0.0;
			}

			void update(Input& input, double deltaTime = Scene::DeltaTime(), const SizeF& sceneSize = Graphics2D::GetRenderTargetSize())
			{
				const auto t1 = Transformer2D(m_defaultGraphLocalTransform, m_defaultCursorLocalTransform, Transformer2D::Target::SetLocal);
				const auto t2 = Transformer2D(m_defaultGraphCameraTransform, m_defaultCursorCameraTransform, Transformer2D::Target::SetCamera);

				if (KeyControl.pressed() && Key0.down())
				{
					setTargetScale(1.0);
				}

				updateWheel(sceneSize);
				updateMouse(input);

				m_scale = Math::SmoothDamp(m_scale, m_targetScale, m_scaleChangeVelocity, m_setting.scaleSmoothTime, deltaTime);

				if (m_pointedScale)
				{
					const Vec2 v = m_pointedScale->first - (sceneSize * 0.5);
					m_targetCenter = m_center = (m_pointedScale->second - v / m_scale);
				}
				else
				{
					m_center = Math::SmoothDamp(m_center, m_targetCenter, m_positionChangeVelocity, m_setting.positionSmoothTime, deltaTime);
				}
			}
		};
	}

	class NodeEditor
	{
	private:

		enum class GrabTarget
		{
			None, Output, Input
		};

		Array<std::shared_ptr<INode>> m_nodelist;

		uint32 m_nextId = 1;

		std::shared_ptr<ISocket> m_grabFrom;

		RenderTexture m_texture;

		Config m_config;

		Input m_input;

		detail::INodeGenerator m_inodeGenerator;

		detail::NodeListWindow m_nodelistWindow;

		detail::EditorCamera2D m_camera = detail::EditorCamera2D({ 0, 0 });

		std::shared_ptr<ISocket> m_candidateSocket;//接続先の候補(見つからないときはnullptr)

		//ケーブルの更新
		void updateCables()
		{
			m_candidateSocket = nullptr;

			if (m_grabFrom)
			{
				//接続の候補を検索
				std::for_each(std::rbegin(m_nodelist), std::rend(m_nodelist), [this](auto& node)
					{
						for (auto& socket : node->getSockets())
						{
							auto pos = socket->calcPos(m_config);
							auto circle = Circle(pos, m_config.ConnectorSize / 2);

							if (m_grabFrom->canConnect(*socket) && m_input.mouseOver(circle))
							{
								m_candidateSocket = socket;
							}
						}
					});
			}
			else
			{
				std::for_each(std::rbegin(m_nodelist), std::rend(m_nodelist), [this](auto& node)
					{
						for (auto& socket : node->getSockets())
						{
							auto pos = socket->calcPos(m_config);
							auto circle = Circle(pos, m_config.ConnectorSize / 2);

							if (m_input.leftClicked(circle))
							{
								//編集開始
								m_grabFrom = socket;
							}
							else if (m_input.rightClicked(circle))
							{
								//接続解除
								ISocket::disconnect(socket);
							}
						}
					});
			}

			if (m_grabFrom)
			{
				if (!MouseL.pressed())
				{
					if (m_candidateSocket)
					{
						ISocket::connect(m_grabFrom, m_candidateSocket);
					}
					else
					{
						m_nodelistWindow.show(Cursor::PosF());
					}
					m_grabFrom = nullptr;
				}
			}
		}

		void drawCable(const Vec2& start, const Vec2& end)
		{
			Bezier3(start, start + Vec2(m_config.BezierX, 0), end + Vec2(-m_config.BezierX, 0), end).draw(2, Palette::White);
		}

		//ケーブルの描画
		void drawCables()
		{
			Vec2 start, end;
			for (auto& node : m_nodelist)
			{
				for (auto& inSocket : node->getAllInputSockets())
				{
					auto outSockets = inSocket->ConnectedSocket;
					for (const auto& outSocket : outSockets)
					{
						start = outSocket->calcPos(m_config);
						end = inSocket->calcPos(m_config);
						drawCable(start, end);
					}
				}
			}
			if (m_grabFrom)
			{
				switch (m_grabFrom->SocketType)
				{
				case IOType::Input:
				{
					start = m_candidateSocket ? m_candidateSocket->calcPos(m_config) : Cursor::PosF();
					end = m_grabFrom->calcPos(m_config);
				}
				break;
				case IOType::Output:
				{
					start = m_grabFrom->calcPos(m_config);
					end = m_candidateSocket ? m_candidateSocket->calcPos(m_config) : Cursor::PosF();
				}
				break;
				}
				drawCable(start, end);
			}
		}

		//ノードの更新
		void updateNodes()
		{
			std::for_each(std::rbegin(m_nodelist), std::rend(m_nodelist), [this](auto& node)
				{
					node->update(m_config, m_input);
				});
		}

		//ノードの描画
		void drawNodes()
		{
			for (auto& node : m_nodelist)
			{
				node->draw(m_config);
			}
		}

		void addNode(std::shared_ptr<INode> node, const Vec2& pos = Vec2(0, 0))
		{
			m_nodelist << node;
			node->ID = m_nextId++;
			node->Location = pos;
		}

	public:

		NodeEditor(Size size)
			:m_nodelistWindow(m_inodeGenerator)
		{
			resize(size);
		}

		void resize(Size size)
		{
			m_texture = RenderTexture(size);
		}

		/// <summary>
		/// 描画,更新処理
		/// </summary>
		/// <param name="drawArea">エディタを表示する範囲</param>
		void draw(const Vec2 location)
		{
			m_input.start();
			if (!RectF(location, m_texture.size()).mouseOver())
			{
				m_input.setProc();
			}
			{
				const ScopedRenderTarget2D renderTarget(m_texture);
				const ScopedViewport2D viewport(0, 0, m_texture.size());
				const Transformer2D transformMouse(Mat3x2::Identity(), Mat3x2::Translate(location));//マウス位置補正

				m_camera.setDefaultTransform();

				Rect(0, 0, m_texture.size()).draw(ColorF(0.3));

				try
				{
					const Transformer2D transformCam(m_camera.getMat3x2(), true);

					auto node = m_nodelistWindow.update(m_config, m_input);
					if (node)
					{
						addNode(node, m_nodelistWindow.m_location);
					}

					updateNodes();

					updateCables();

					m_camera.update(m_input);

					drawCables();

					drawNodes();

					m_nodelistWindow.draw(m_config);
				}
				catch (Error& ex)
				{
					m_config.font(ex.what()).drawAt(m_texture.size() / 2, Palette::Black);
				}
			}
			m_texture.draw(location);
		}

		template<class NodeType>
		void registerNodeType()
		{
			m_inodeGenerator.registerType<NodeType>();
		}

		template<class FuncType>
		void registerNodeFunction(const String& name, const Array<String>& argNames, const std::function<FuncType>& function)
		{
			m_inodeGenerator.registerFunction<FuncType>(name, argNames, function);
		}

		template<class NodeType>
		Optional<std::shared_ptr<NodeType>> addNode(const Vec2& pos = Vec2(0, 0))
		{
			auto result = addNode(Type::getType<NodeType>(), pos);
			if (result)
			{
				return std::dynamic_pointer_cast<NodeType>(*result);
			}
			else
			{
				return none;
			}
		}

		Optional<std::shared_ptr<INode>> addNode(const String& name, const Vec2& pos = Vec2(0, 0))
		{
			auto inode = m_inodeGenerator.getINode(name);
			if (inode)
			{
				m_nodelist << *inode;
				(*inode)->ID = m_nextId++;
				(*inode)->Location = pos;
			}
			return inode;
		}

		Optional<std::shared_ptr<INode>> addNode(const Type& type, const Vec2& pos = Vec2(0, 0))
		{
			auto inode = m_inodeGenerator.getINode(type);
			if (inode)
			{
				m_nodelist << *inode;
				(*inode)->ID = m_nextId++;
				(*inode)->Location = pos;
			}
			return inode;
		}

		Optional<std::shared_ptr<INode>> searchNode(size_t id)
		{
			for (auto& node : m_nodelist)
			{
				if (node->ID == id)
				{
					return node;
				}
			}
			return none;
		}

		Optional<std::shared_ptr<INode>> searchNode(const String& className)
		{
			for (auto& node : m_nodelist)
			{
				if (node->Class == className)
				{
					return node;
				}
			}
			return none;
		}

		String save()
		{
			JSONWriter writer;

			writer.startObject();
			{
				writer.key(U"nodes").startArray();
				for (const auto& node : m_nodelist)
				{
					node->serialize(writer);
				}
				writer.endArray();
			}
			writer.endObject();

			return writer.get();
		}

		void load(const JSONReader& json)
		{
			auto nodes = json[U"nodes"].arrayView();
			auto nodesCount = json[U"nodes"].arrayCount();

			m_nodelist.resize(nodesCount);

			for (size_t i = 0; i < nodesCount; i++)
			{
				auto className = nodes[i][U"class"].getString();
				auto inode = m_inodeGenerator.getINode(className);
				if (inode)
				{
					auto node = (*inode);
					node->deserialize(nodes[i]);
					m_nodelist[i] = node;
				}
				else
				{
					throw new Error(U"クラス\"{}\"が見つかりませんでした"_fmt(className));
				}
			}

			for (size_t i = 0; i < nodesCount; i++)
			{
				auto node = m_nodelist[i];
				node->deserializeSockets(nodes[i], m_nodelist);
			}
		}
	};
}
