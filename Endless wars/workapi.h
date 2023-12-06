#pragma once

#ifdef WORKAPI_EXPORTS
#define WORKAPI_API _declspec(dllexport)
#else 
#define WORKAPI_API _declspec(dllimport)
#endif

enum class dirs { stop = 0, left = 1, right = 2, up = 3, down = 4 };
enum class types { good = 0, bad = 1 };
enum class port_types { port10 = 0, port20 = 1, port30 = 2, port40 = 3 };

class WORKAPI_API OBJECT
{
	protected:
		float width = 0.0f;
		float height = 0.0f;

	public:
		float x = 0.0f;
		float y = 0.0f;
		float ex = 0.0f;
		float ey = 0.0f;

		dirs dir = dirs::stop;

		OBJECT(float _x, float _y, float _width, float _height)
		{
			x = _x;
			y = _y;
			width = _width;
			height = _height;
			ex = x + width;
			ey = y + height;
		}

		virtual ~OBJECT() {};

		void SetDims()
		{
			ex = x + width;
			ey = y + height;
		}

		void NewDims(float _width, float _height)
		{
			width = _width;
			height = _height;
			ex = x + width;
			ey = y + height;
		}

		virtual void Release()
		{
			delete this;
		}

};
class WORKAPI_API WARRIOR :public OBJECT
{
	protected:
		int max_frames = 7;
		int frame_count = -1;

		float slope = 1.0f;
		float intercept = 0;
		
		float target_x = 0;
		float target_y = 0;
		
	public:
		types type = types::good;
		bool obstacle = false;
		float speed = 0.7f;

		WARRIOR(float _where_x, float _where_y) :OBJECT(_where_x, _where_y, 40.0f, 30.0f)
		{
			slope = 1.0f;
			speed = 0.2f;
			target_x = 0;
			target_y = 0;
		}

		virtual ~WARRIOR() {};

		virtual void Release() = 0;
		virtual void SetSlope() = 0;
		virtual void Move(float _tar_x = 0, float _tar_y = 0) = 0;
		virtual int GetFrame() = 0;
		virtual bool OutOfScreen(float _x, float _y, bool check_ex, bool check_ey) const = 0;
};
class WORKAPI_API PORTAL :public OBJECT
{
	public:
		port_types type = port_types::port10;
		int multiplier = 10;

		PORTAL(port_types _type, float _x, float _y) :OBJECT(_x, _y, 100.0f, 94.0f)
		{
			type = _type;
			switch (type)
			{
				case port_types::port10:
					multiplier = 10;
					break;

				case port_types::port20:
					multiplier = 20;
					break;

				case port_types::port30:
					multiplier = 30;
					break;

				case port_types::port40:
					multiplier = 40;
					break;
			}
		}
		void Release()
		{
			delete this;
		}
};

typedef WARRIOR* Warrior;
typedef OBJECT* Object;
typedef PORTAL* Portal;

extern WORKAPI_API Warrior iCreateWarrior(types _which_type, float _x, float _y);