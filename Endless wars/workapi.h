#pragma once

#ifdef WORKAPI_EXPORTS
#define WORKAPI_API _declspec(dllexport)
#else 
#define WORKAPI_API _declspec(dllimport)
#endif

enum class dirs { stop = 0, left = 1, right = 2, up = 3, down = 4 };
enum class types { good = 0, bad = 1 };

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
		float speed = 0.2f;
		float target_x = 0;
		float target_y = 0;
		bool slope_set = false;

	public:
		types type = types::good;
		bool obstacle = false;

		WARRIOR(float _where_x, float _where_y) :OBJECT(_where_x, _where_y, 40.0f, 30.0f)
		{
			slope = 1.0f;
			speed = 1.0f;
			target_x = 0;
			target_y = 0;
		}

		virtual ~WARRIOR() {};

		virtual void Release() = 0;
		virtual float SetSlope() = 0;
		virtual void Move(float _tar_x = 0, float _tar_y = 0) = 0;
		virtual int GetFrame() = 0;
		virtual bool OutOfScreen(float _x, float _y, bool check_ex, bool check_ey) const = 0;
};

typedef WARRIOR* Warrior;
typedef OBJECT* Object;

extern WORKAPI_API Warrior iCreateWarrior(types _which_type, float _x, float _y);