#ifndef _CIRCULAR_H_
#define _CIRCULAR_H_
/********************************************************************/
/********************************************************************/
/*         Generated Filter by CircularDofFilterGenerator tool      */
/*     Copyright (c)     Kleber A Garcia  (kecho_garcia@hotmail.com)*/
/*       https://github.com/kecho/CircularDofFilterGenerator        */
/********************************************************************/
/********************************************************************/
/**
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. 
**/
const int KERNEL_RADIUS = 8;
const int KERNEL_COUNT = 17;

const vec4 Kernel0BracketsRealXY_ImZW_1 = vec4(-0.015162,0.864207,0.000000,0.362356);
const vec2 Kernel0Weights_RealX_ImY_1 = vec2(0.767583,1.862321);
const vec4 Kernel0_RealX_ImY_RealZ_ImW_1[] = vec4[](
	vec4(/*XY: Non Bracketed*/-0.015162,0.015669,/*Bracketed WZ:*/0.000000,0.043242),
	vec4(/*XY: Non Bracketed*/-0.006382,0.028463,/*Bracketed WZ:*/0.010160,0.078550),
	vec4(/*XY: Non Bracketed*/0.009457,0.036328,/*Bracketed WZ:*/0.028488,0.100254),
	vec4(/*XY: Non Bracketed*/0.028374,0.036801,/*Bracketed WZ:*/0.050378,0.101559),
	vec4(/*XY: Non Bracketed*/0.046136,0.030553,/*Bracketed WZ:*/0.070930,0.084319),
	vec4(/*XY: Non Bracketed*/0.059985,0.020481,/*Bracketed WZ:*/0.086955,0.056523),
	vec4(/*XY: Non Bracketed*/0.069097,0.010177,/*Bracketed WZ:*/0.097499,0.028086),
	vec4(/*XY: Non Bracketed*/0.073979,0.002706,/*Bracketed WZ:*/0.103148,0.007467),
	vec4(/*XY: Non Bracketed*/0.075479,0.000000,/*Bracketed WZ:*/0.104884,0.000000),
	vec4(/*XY: Non Bracketed*/0.073979,0.002706,/*Bracketed WZ:*/0.103148,0.007467),
	vec4(/*XY: Non Bracketed*/0.069097,0.010177,/*Bracketed WZ:*/0.097499,0.028086),
	vec4(/*XY: Non Bracketed*/0.059985,0.020481,/*Bracketed WZ:*/0.086955,0.056523),
	vec4(/*XY: Non Bracketed*/0.046136,0.030553,/*Bracketed WZ:*/0.070930,0.084319),
	vec4(/*XY: Non Bracketed*/0.028374,0.036801,/*Bracketed WZ:*/0.050378,0.101559),
	vec4(/*XY: Non Bracketed*/0.009457,0.036328,/*Bracketed WZ:*/0.028488,0.100254),
	vec4(/*XY: Non Bracketed*/-0.006382,0.028463,/*Bracketed WZ:*/0.010160,0.078550),
	vec4(/*XY: Non Bracketed*/-0.015162,0.015669,/*Bracketed WZ:*/0.000000,0.043242)
);

const vec4 Kernel0BracketsRealXY_ImZW_2 = vec4(-0.045884,1.097245,-0.033796,0.838935);
const vec2 Kernel0Weights_RealX_ImY_2 = vec2(0.411259,-0.548794);
const vec4 Kernel0_RealX_ImY_RealZ_ImW_2[] = vec4[](
	vec4(/*XY: Non Bracketed*/0.005645,0.020657,/*Bracketed WZ:*/0.046962,0.064907),
	vec4(/*XY: Non Bracketed*/0.025697,-0.013190,/*Bracketed WZ:*/0.065237,0.024562),
	vec4(/*XY: Non Bracketed*/-0.016100,-0.033796,/*Bracketed WZ:*/0.027145,0.000000),
	vec4(/*XY: Non Bracketed*/-0.045884,0.008247,/*Bracketed WZ:*/0.000000,0.050114),
	vec4(/*XY: Non Bracketed*/-0.017867,0.052848,/*Bracketed WZ:*/0.025534,0.103278),
	vec4(/*XY: Non Bracketed*/0.030969,0.056175,/*Bracketed WZ:*/0.070042,0.107244),
	vec4(/*XY: Non Bracketed*/0.063053,0.032363,/*Bracketed WZ:*/0.099282,0.078860),
	vec4(/*XY: Non Bracketed*/0.074716,0.008899,/*Bracketed WZ:*/0.109911,0.050892),
	vec4(/*XY: Non Bracketed*/0.076760,0.000000,/*Bracketed WZ:*/0.111774,0.040284),
	vec4(/*XY: Non Bracketed*/0.074716,0.008899,/*Bracketed WZ:*/0.109911,0.050892),
	vec4(/*XY: Non Bracketed*/0.063053,0.032363,/*Bracketed WZ:*/0.099282,0.078860),
	vec4(/*XY: Non Bracketed*/0.030969,0.056175,/*Bracketed WZ:*/0.070042,0.107244),
	vec4(/*XY: Non Bracketed*/-0.017867,0.052848,/*Bracketed WZ:*/0.025534,0.103278),
	vec4(/*XY: Non Bracketed*/-0.045884,0.008247,/*Bracketed WZ:*/0.000000,0.050114),
	vec4(/*XY: Non Bracketed*/-0.016100,-0.033796,/*Bracketed WZ:*/0.027145,0.000000),
	vec4(/*XY: Non Bracketed*/0.025697,-0.013190,/*Bracketed WZ:*/0.065237,0.024562),
	vec4(/*XY: Non Bracketed*/0.005645,0.020657,/*Bracketed WZ:*/0.046962,0.064907)
);
const vec4 Kernel1BracketsRealXY_ImZW_2 = vec4(-0.002843,0.595479,0.000000,0.189160);
const vec2 Kernel1Weights_RealX_ImY_2 = vec2(0.513282,4.561110);
const vec4 Kernel1_RealX_ImY_RealZ_ImW_2[] = vec4[](
	vec4(/*XY: Non Bracketed*/-0.002843,0.003566,/*Bracketed WZ:*/0.000000,0.018854),
	vec4(/*XY: Non Bracketed*/-0.001296,0.008744,/*Bracketed WZ:*/0.002598,0.046224),
	vec4(/*XY: Non Bracketed*/0.004764,0.014943,/*Bracketed WZ:*/0.012775,0.078998),
	vec4(/*XY: Non Bracketed*/0.016303,0.019581,/*Bracketed WZ:*/0.032153,0.103517),
	vec4(/*XY: Non Bracketed*/0.032090,0.020162,/*Bracketed WZ:*/0.058664,0.106584),
	vec4(/*XY: Non Bracketed*/0.049060,0.016015,/*Bracketed WZ:*/0.087162,0.084666),
	vec4(/*XY: Non Bracketed*/0.063712,0.008994,/*Bracketed WZ:*/0.111767,0.047547),
	vec4(/*XY: Non Bracketed*/0.073402,0.002575,/*Bracketed WZ:*/0.128041,0.013610),
	vec4(/*XY: Non Bracketed*/0.076760,0.000000,/*Bracketed WZ:*/0.133679,0.000000),
	vec4(/*XY: Non Bracketed*/0.073402,0.002575,/*Bracketed WZ:*/0.128041,0.013610),
	vec4(/*XY: Non Bracketed*/0.063712,0.008994,/*Bracketed WZ:*/0.111767,0.047547),
	vec4(/*XY: Non Bracketed*/0.049060,0.016015,/*Bracketed WZ:*/0.087162,0.084666),
	vec4(/*XY: Non Bracketed*/0.032090,0.020162,/*Bracketed WZ:*/0.058664,0.106584),
	vec4(/*XY: Non Bracketed*/0.016303,0.019581,/*Bracketed WZ:*/0.032153,0.103517),
	vec4(/*XY: Non Bracketed*/0.004764,0.014943,/*Bracketed WZ:*/0.012775,0.078998),
	vec4(/*XY: Non Bracketed*/-0.001296,0.008744,/*Bracketed WZ:*/0.002598,0.046224),
	vec4(/*XY: Non Bracketed*/-0.002843,0.003566,/*Bracketed WZ:*/0.000000,0.018854)
);
#endif