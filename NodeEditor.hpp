#pragma once
#include<Siv3D.hpp>
#include<regex>
#include"Input.hpp"
#include"Config.hpp"
#include"Node.hpp"
#include"NodeSocket.hpp"
#include"Group.hpp"

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

		class NodeGenerator
		{
		private:

			using GeneratorType = std::function<std::shared_ptr<Node>(void)>;

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

			struct NodeClass
			{
				bool isFunction;
				bool visible;
				GeneratorType generator;
			};

			struct Group
			{
				std::unordered_map<String, NodeClass> classes;
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
			void registerType(bool visible = true)
			{
				Type type = Type::getType<SubType>();

				auto names = parseNames(Unicode::FromUTF8(typeid(SubType).name()));

				auto targetNamespace = std::ref(global);
				for (size_t i = 0; i < names.size() - 1; i++)
				{
					targetNamespace = targetNamespace.get().namespaces[names[i]];
				}
				targetNamespace.get().classes.emplace(names[names.size() - 1], NodeClass{ false,visible,createGenerator<SubType>(names.join(U"::",U"",U"")) });
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
				targetNamespace.get().classes.emplace(names[names.size() - 1], NodeClass{ true,true,createFuncGenerator<FuncType>(names.join(U"::",U"",U""),names[names.size() - 1],argNames,function) });
			}

			Optional<std::shared_ptr<Node>> getNode(const Type& type)
			{
				return getNode(parseNames(type.name()));
			}

			Optional<std::shared_ptr<Node>> getNode(const String& names)
			{
				return getNode(parseNames(names));
			}

			Optional<std::shared_ptr<Node>> getNode(const Array<String>& names)
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

			Vec2 m_windowLocation;

			const NodeGenerator& m_generator;

			const Texture m_nsTexture = Texture(U"icons/namespace.png");

			const Texture m_classTexture = Texture(U"icons/class.png");

			const Texture m_functionTexture = Texture(U"icons/function.png");

			const Texture m_anglerightTexture = Texture(Icon(0xf105, 16));

			std::pair<String, NodeGenerator::Group> m_currentNs;

		public:

			Vec2 m_nodeLocation;

			NodeListWindow(const NodeGenerator& generator)
				:m_generator(generator)
			{

			}

			void show(const Vec2& pos)
			{
				m_currentNs = { U"global",m_generator.global };
				m_nodeLocation = pos;
				m_visible = true;
			}

			void hide()
			{
				m_visible = false;
			}

			std::shared_ptr<Node> update(const Config& cfg, Input& input, const Mat3x2& mat)
			{
				std::shared_ptr<Node> result;

				if (m_visible)
				{
					m_windowLocation = mat.transform(m_nodeLocation);

					const RectF rect(m_windowLocation, width, cfg.font.height() * (lineCnt + 1));
					const RectF contentRect(m_windowLocation.x, m_windowLocation.y + cfg.font.height(), width, cfg.font.height() * lineCnt);
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
						if (keyval.second.visible)
						{
							if (input.leftClicked(RectF(fontPos, width, cfg.font.height())))
							{
								result = keyval.second.generator();
								hide();
								goto exitInput;
							}
							fontPos.y += cfg.font.height();
						}
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
					const RectF rect(m_windowLocation, width, cfg.font.height() * (lineCnt + 1));
					const RectF titleRect(m_windowLocation, width, cfg.font.height());
					const RectF contentRect(m_windowLocation.x, m_windowLocation.y + cfg.font.height(), width, cfg.font.height() * lineCnt);

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
						if (keyval.second.visible)
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
				const double wheelY = Mouse::Wheel();
				const double wheelX = Mouse::WheelH();

				if (KeyControl.pressed())
				{
					if (wheelY == 0.0)
					{
						return;
					}

					m_positionChangeVelocity = Vec2::Zero();

					if (wheelY < 0.0)
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
				else
				{
					m_targetCenter += Vec2(wheelX, wheelY) * 10 / m_scale;
				}
			}

			void updateMouse(Input& input)
			{
				if (input.down(MouseM))
				{
					m_grab = true;
					m_pointedScale.reset();
				}
				else if (m_grab)
				{
					m_targetCenter -= Cursor::DeltaF() / m_scale;

					if (MouseM.up())
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

			void update(Input& input, const SizeF& sceneSize = Graphics2D::GetRenderTargetSize(), double deltaTime = Scene::DeltaTime())
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

		int32 m_updateFrameCnt = -1;

		Array<std::shared_ptr<Node>> m_nodelist;

		Array<std::shared_ptr<Group>> m_grouplist;

		uint32 m_nextId = 1;

		std::shared_ptr<ISocket> m_grabFrom;

		bool m_isGrab = false;

		bool m_rangeSelection = false;

		bool m_rangeSelectionIsGroup = false;

		Vec2 m_rangeSelectionBegin;

		RectF m_rangeSelectionRange;

		RenderTexture m_texture;

		Config m_config;

		Input m_input;

		detail::NodeGenerator m_inodeGenerator;

		detail::NodeListWindow m_nodelistWindow;

		detail::EditorCamera2D m_camera = detail::EditorCamera2D({ 0, 0 });

		std::shared_ptr<ISocket> m_candidateSocket;//接続先の候補(見つからないときはnullptr)

		void deselectAll()
		{
			for (auto& node : m_nodelist)
			{
				node->Selecting = false;
			}
		}

		//ケーブルの更新
		void updateCables()
		{
			m_candidateSocket = nullptr;

			if (m_isGrab)
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
								m_isGrab = true;
							}
							else if (m_input.rightClicked(circle))
							{
								//接続解除
								ISocket::disconnect(socket);
							}
						}
					});
			}

			if (m_isGrab)
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
					m_isGrab = false;
				}
			}
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
			if (m_isGrab)
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

		void drawCable(const Vec2& start, const Vec2& end)
		{
			Bezier3(start, start + Vec2(m_config.BezierX, 0), end + Vec2(-m_config.BezierX, 0), end).draw(2, Palette::White);
		}

		//ノードの更新
		void updateNodes()
		{
			bool selectAppend = KeyShift.pressed() || KeyControl.pressed();
			std::for_each(std::rbegin(m_nodelist), std::rend(m_nodelist), [&](std::shared_ptr<Node>& node)
				{
					node->update(m_config, m_input);
					if (node->clicked())
					{
						if (!selectAppend)
						{
							deselectAll();
						}
						node->Selecting = !node->Selecting;
					}
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

		//範囲選択の更新
		void updateRangeSelection()
		{
			m_rangeSelectionIsGroup = KeyControl.pressed();
			if (!m_input.getProc() && MouseL.down())
			{
				deselectAll();
				m_rangeSelection = true;
				m_rangeSelectionBegin = Cursor::PosF();
			}

			if (m_rangeSelection)
			{
				if (MouseL.pressed())
				{
					auto cursor = Cursor::PosF();
					m_rangeSelectionRange = RectF(
						Min(cursor.x, m_rangeSelectionBegin.x),
						Min(cursor.y, m_rangeSelectionBegin.y),
						Abs(cursor.x - m_rangeSelectionBegin.x),
						Abs(cursor.y - m_rangeSelectionBegin.y)
					);
					for (auto& node : m_nodelist)
					{
						node->Selecting = m_rangeSelectionRange.contains(node->getRect());
					}
				}
				else
				{
					m_rangeSelection = false;
					if (m_rangeSelectionIsGroup)
					{
						auto group = std::make_shared<Group>();
						group->Rect = m_rangeSelectionRange;
						group->Name = U"Group";
						m_grouplist << group;
					}
				}
			}
		}

		//範囲選択の描画
		void drawRangeSelection()
		{
			if (m_rangeSelection)
			{
				if (m_rangeSelectionIsGroup)
				{
					Group group;
					group.Rect = m_rangeSelectionRange;
					group.draw(m_config);
				}
				else
				{
					m_rangeSelectionRange.draw(ColorF(0.4)).drawFrame(2, ColorF(0.44));
				}
			}
		}

		//グループの更新
		void updateGroups()
		{
			std::for_each(std::rbegin(m_grouplist), std::rend(m_grouplist), [&](std::shared_ptr<Group>& group)
				{
					group->update(m_config, m_input, m_nodelist);
				});
		}

		//グループの描画
		void drawGroups()
		{
			for (auto& group : m_grouplist)
			{
				group->draw(m_config);
			}
		}

		//キー入力の更新
		void updateKeyInput()
		{
			if (KeyDelete.down())
			{
				m_nodelist.remove_if([this](std::shared_ptr<Node> node)
					{
						bool result = node->canDelete() && node->Selecting;
						if (result)
						{
							node->disconnectAllSockets();
						}
						return result;
					});
			}
		}

		void addNode(std::shared_ptr<Node> node, const Vec2& pos = Vec2(0, 0))
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
		/// 更新処理
		/// </summary>
		/// <param name="location">エディタを表示する位置</param>
		void update(const Vec2 location)
		{
			m_updateFrameCnt = Scene::FrameCount();
			m_input.start();
			if (!RectF(location, m_texture.size()).mouseOver())
			{
				m_input.setProc();
			}
			{
				const ScopedRenderTarget2D renderTarget(m_texture);
				const ScopedViewport2D viewport(0, 0, m_texture.size());
				const Transformer2D transformCamera(Mat3x2::Identity(), Mat3x2::Identity(), Transformer2D::Target::SetCamera);
				const Transformer2D transformMouse(Mat3x2::Identity(), Mat3x2::Translate(location));//マウス位置補正

				m_camera.setDefaultTransform();

				{
					auto node = m_nodelistWindow.update(m_config, m_input, m_camera.getMat3x2());
					if (node)
					{
						addNode(node, m_nodelistWindow.m_nodeLocation);
						if (m_grabFrom)
						{
							for (auto socket : node->getSockets())
							{
								if (m_grabFrom->canConnect(*socket))
								{
									ISocket::connect(m_grabFrom, socket);
								}
							}
							m_grabFrom = nullptr;
						}
					}
				}
				{
					const Transformer2D transformCam(m_camera.getMat3x2(), true);

					updateNodes();

					updateCables();

					updateGroups();

					updateRangeSelection();

					m_camera.update(m_input, m_texture.size());

					updateKeyInput();
				}
			}
		}

		/// <summary>
		/// 描画(更新)処理
		/// </summary>
		/// <param name="location">エディタを表示する位置</param>
		void draw(const Vec2 location)
		{
			//更新処理をする前に呼び出された場合、更新処理をする
			if (m_updateFrameCnt != Scene::FrameCount())
			{
				update(location);
			}

			{
				const ScopedRenderTarget2D renderTarget(m_texture);
				const ScopedViewport2D viewport(0, 0, m_texture.size());
				const Transformer2D transformCamera(Mat3x2::Identity(), Mat3x2::Identity(), Transformer2D::Target::SetCamera);
				const Transformer2D transformMouse(Mat3x2::Identity(), Mat3x2::Translate(location));//マウス位置補正

				Rect(0, 0, m_texture.size()).draw(ColorF(0.3));

				{
					const Transformer2D transformCam(m_camera.getMat3x2(), true);

					drawGroups();

					drawRangeSelection();

					drawCables();

					drawNodes();
				}
				m_nodelistWindow.draw(m_config);
			}
			m_texture.draw(location);
		}

		template<class NodeType>
		void registerNodeType(bool visible = true)
		{
			m_inodeGenerator.registerType<NodeType>(visible);
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

		Optional<std::shared_ptr<Node>> addNode(const String& name, const Vec2& pos = Vec2(0, 0))
		{
			auto inode = m_inodeGenerator.getNode(name);
			if (inode)
			{
				m_nodelist << *inode;
				(*inode)->ID = m_nextId++;
				(*inode)->Location = pos;
			}
			return inode;
		}

		Optional<std::shared_ptr<Node>> addNode(const Type& type, const Vec2& pos = Vec2(0, 0))
		{
			auto inode = m_inodeGenerator.getNode(type);
			if (inode)
			{
				m_nodelist << *inode;
				(*inode)->ID = m_nextId++;
				(*inode)->Location = pos;
			}
			return inode;
		}

		Optional<std::shared_ptr<Node>> searchNode(size_t id)
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

		Optional<std::shared_ptr<Node>> searchNode(const String& className)
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

		void clear()
		{
			m_nextId = 1;
			m_nodelist.clear();
			m_grouplist.clear();
			m_grabFrom = nullptr;
			m_isGrab = false;
			m_camera.setScale(1.0);
			m_camera.setCenter({ 0,0 });
			m_candidateSocket = nullptr;
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
			clear();

			auto nodes = json[U"nodes"].arrayView();
			auto nodesCount = json[U"nodes"].arrayCount();

			m_nodelist.resize(nodesCount);

			for (size_t i = 0; i < nodesCount; i++)
			{
				auto className = nodes[i][U"class"].getString();
				auto inode = m_inodeGenerator.getNode(className);
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
