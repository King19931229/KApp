#define SH_BINDING_COEFFICIENT 0
#define SH_BINDING_CUBEMAP 1
#define SH_GROUP_SIZE 16

layout(local_size_x = SH_GROUP_SIZE, local_size_y = SH_GROUP_SIZE, local_size_z = 1) in;

vec3 centers[6] = 	{ vec3(1, 0, 0), vec3(-1, 0, 0), vec3(0, 1, 0), vec3(0, -1, 0), vec3(0, 0, 1), vec3(0, 0, -1) };
vec3 rights[6] 	=  	{ vec3(0, 0, -1), vec3(0, 0, 1), vec3(1, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0), vec3(-1, 0, 0) };
vec3 ups[6] 	= 	{ vec3(0, -1, 0), vec3(0, -1, 0), vec3(0, 0, 1), vec3(0, 0, -1), vec3(0, -1, 0), vec3(0, -1, 0) };

const float PI = 3.141592654;
const float SQRT_PI = sqrt(PI);
const float SQRT_3 = sqrt(3);
const float SQRT_5 = sqrt(5);
const float SQRT_15 = sqrt(15);

const float S0 = 0.5 / SQRT_PI;
const float S1 = 0.5 * SQRT_3 / SQRT_PI;
const float S2 = 0.5 * SQRT_15 / SQRT_PI;
const float S3 = 0.25 * SQRT_5 / SQRT_PI;
const float sh_basics[9] = { S0, -S1, S1, -S1, S2, -S2, S3, -S2, S3 };

void EvalSH(vec3 pos, inout float s[9])
{
	s[0] = sh_basics[0];
	s[1] = sh_basics[1] * pos.y;
	s[2] = sh_basics[2] * pos.z;
	s[3] = sh_basics[3] * pos.x;
	s[4] = sh_basics[4] * pos.x * pos.y;
	s[5] = sh_basics[5] * pos.y * pos.z;
	s[6] = sh_basics[6] * (3.0 * pos.z * pos.z - 1.0);
	s[7] = sh_basics[7] * pos.x * pos.z;
	s[8] = sh_basics[8] * (pos.x * pos.x - pos.y * pos.y);
}