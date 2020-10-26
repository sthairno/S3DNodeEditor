#pragma once

namespace NodeEditor
{
	/// <summary>
	/// 入力補助クラス
	/// </summary>
	class Input
	{
	private:

		bool m_proc = false;

		bool updateProc()
		{
			if (m_proc)
			{
				return false;
			}
			m_proc = true;
			return true;
		}

	public:

		void start()
		{
			m_proc = false;
		}

		void setProc()
		{
			m_proc = true;
		}

		bool getProc()
		{
			return m_proc;
		}

		template<class Shape>
		bool leftClicked(const Shape& obj)
		{
			return obj.leftClicked() && updateProc();
		}

		template<class Shape>
		bool leftPressed(const Shape& obj)
		{
			return obj.leftPressed() && updateProc();
		}

		template<class Shape>
		bool leftReleased(const Shape& obj)
		{
			return obj.leftReleased();
		}

		template<class Shape>
		bool rightClicked(const Shape& obj)
		{
			return obj.rightClicked() && updateProc();
		}

		template<class Shape>
		bool rightPressed(const Shape& obj)
		{
			return obj.rightPressed() && updateProc();
		}

		template<class Shape>
		bool rightReleased(const Shape& obj)
		{
			return obj.rightReleased();
		}

		template<class Shape>
		bool mouseOver(const Shape& obj) const
		{
			return !m_proc && obj.mouseOver();
		}

		bool down(const Key& key)
		{
			return key.down() && updateProc();
		}
	};
}