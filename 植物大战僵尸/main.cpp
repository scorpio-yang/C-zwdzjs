#include<stdio.h>
#include<graphics.h>  //easyx图形库的头文件
#include<time.h>
#include<math.h>
#include"tools.h"
#include"vector2.h"

#include<mmsystem.h>  //播放音乐头文件
#pragma comment(lib, "winmm.lib")  //告诉编译器，加载winmm. lib库文件 

#define WIN_WIDTH	1000
#define WIN_HEIGHT	600
#define ZM_MAX		10

enum { WAN_DOU, XIANG_RI_KUI, ZHI_WU_COUNT };

IMAGE imgBg;  //表示背景图片；IMAGE为easyx中的图像类型变量
IMAGE imgBar;  //工具栏图片
IMAGE imgCards[ZHI_WU_COUNT];  //植物卡片
IMAGE* imgZhiWu[ZHI_WU_COUNT][20];  //拖动中的图片

int curX, curY;  //当前选中的植物，在移动过程中的位置
int curZhiWu;  //0:没有选中 1:表示选择了第一种植物 2:选中了第二种植物

enum { GOING, WIN, FAIL };
int zmCount;  //已经出现的僵尸个数
int killCount;  //已经杀掉的僵尸数量
int gameStatus;  //游戏状态

struct zhiwu {
	int type;  //0:没有选中 1:表示选择了第一种植物 2:选中了第二种植物
	int flameIndex;  //序列帧的序号 
	bool catched;  //植物是否被僵尸捕获
	int deadTime;  //死亡计数器

	int timer;
	int x, y;

	int shootTime;
};      
struct zhiwu map[5][9];

enum { SUNSHINE_DOWN, SUNSHINE_GROUND, SUNSHINE_COLLECT, SUNSHINE_PRODUCT };

struct sunshineBall {
	int x, y;  //阳光球在飘落过程中的坐标位置（x坐标不变）
	int flameIndex;  //当前显示的图片帧的序号
	int destY;  //飘落目标位置的y坐标
	bool used;  //是否在使用
	int timer;  //计时器
	
	float xoff;  //x坐标的偏移量
	float yoff;  //y坐标的偏移量

	float t;  //贝塞尔曲线的时间点 0..1
	vector2 p1, p2, p3, p4;
	vector2 pCur;  //当前时刻阳光球的位置
	float speed;
	int status;
};
struct sunshineBall balls[10];  //阳光池
IMAGE imgSunshineBall[29];  //阳光球的图片数组
int sunShine;

struct zm {
	int x, y;
	int flameIndex;
	bool used;
	int speed;
	int row;
	int blood;
	bool dead;
	bool eating;  //正在吃植物的状态
};
struct zm zms[10];
IMAGE imgZM[22];
IMAGE imgZMDead[20];
IMAGE imgZMEat[21];
IMAGE imgZMStand[11];

struct bullet {
	int x, y;
	int row;
	bool used;
	int speed;
	int flameIndex;  //帧序号
	bool blast;  //豌豆子弹是否爆裂
};
struct bullet bullets[30];
IMAGE imgBulletNormal;
IMAGE imgBulletBlast[4];

bool fileExist(const char* name) {
	FILE* fp;
	fopen_s(&fp,name, "r");
	if (fp == NULL) {
		return false;
	}
	else {
		fclose(fp);
		return true;
	}
}

void gameInit() {
	//加载游戏背景图片
	//把字符集修改为“多字节字符集”
	loadimage(&imgBg, "../植物大战僵尸-素材/res/Map/map0.jpg");

	loadimage(&imgBar, "../植物大战僵尸-素材/res/bar4.png");

	memset(imgZhiWu, 0, sizeof(imgZhiWu));

	memset(map, 0, sizeof(map));

	zmCount = 0;
	killCount = 0;
	gameStatus = GOING;

	//初始化植物卡牌
	char name[64];
	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		//生成植物卡牌的文件名
		sprintf_s(name, sizeof(name), "../植物大战僵尸-素材/res/Cards2/Card2_%d.png", i + 1);
		loadimage(&imgCards[i], name);
		for (int j = 0; j < 20; j++) {
			sprintf_s(name, sizeof(name), "../植物大战僵尸-素材/res/zhiwu/%d/%d.png", i, j + 1);
			//判断这个文件是否存在
			if (fileExist(name)) {
				imgZhiWu[i][j] = new IMAGE;
				loadimage(imgZhiWu[i][j], name);
			}
			else {
				break;
			}
		}
	}

	curZhiWu = 0;
	sunShine = 50;

	//初始化阳光池
	memset(balls, 0, sizeof(balls));
	//初始化阳光球
	for (int i = 0; i < 29; i++) {
		sprintf_s(name, sizeof(name), "../植物大战僵尸-素材/res/sunshine/%d.png", i + 1);
		loadimage(&imgSunshineBall[i], name);
	}

	//配置随机种子
	srand(time(NULL));

	//创建游戏窗口
	initgraph(WIN_WIDTH, WIN_HEIGHT);

	//设置字体
	LOGFONT f;
	gettextstyle(&f);  //获取当前字体
	f.lfHeight = 30;  //修改字体高度
	f.lfWeight = 15;  //修改字体宽度
	strcpy(f.lfFaceName, "Segoe UI Black");  //修改字体样式
	f.lfQuality = ANTIALIASED_QUALITY;  //抗锯齿效果
	settextstyle(&f);
	setbkmode(TRANSPARENT);  //设置字体背景透明
	setcolor(BLACK);  //字体颜色

	//初始化僵尸
	memset(zms, 0, sizeof(zms));
	for (int i = 0; i < 22; i++) {
		sprintf_s(name, sizeof(name), "../植物大战僵尸-素材/res/zm/%d.png", i + 1);
		loadimage(&imgZM[i], name);
	}

	//初始化子弹
	loadimage(&imgBulletNormal, "../植物大战僵尸-素材/res/bullets/bullet_normal.png");
	memset(bullets, 0, sizeof(bullets));

	//初始化豌豆子弹爆裂图片
	loadimage(&imgBulletBlast[3], "../植物大战僵尸-素材/res/bullets/bullet_blast.png");
	for (int i = 0; i < 3; i++) {
		float k = (i + 1) * 0.2;
		loadimage(&imgBulletBlast[i], "../植物大战僵尸-素材/res/bullets/bullet_blast.png",
			imgBulletBlast[3].getwidth() * k, imgBulletBlast[3].getheight() * k, true);
	}

	//初始化僵尸死亡的图片数组
	for (int i = 0; i < 20; i++) {
		sprintf_s(name, sizeof(name), "../植物大战僵尸-素材/res/zm_dead/%d.png", i + 1);
		loadimage(&imgZMDead[i], name);
	}

	//初始化僵尸吃植物的图片数组
	for (int i = 0; i < 21; i++) {
		sprintf_s(name, sizeof(name), "../植物大战僵尸-素材/res/zm_eat/%d.png", i + 1);
		loadimage(&imgZMEat[i], name);
	}

	//初始化过场动画中的僵尸图片
	for (int i = 0; i < 11; i++) {
		sprintf_s(name, sizeof(name), "../植物大战僵尸-素材/res/zm_stand/%d.png", i + 1);
		loadimage(&imgZMStand[i], name);
	}
}

void drawZM() {
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zmCount; i++) {
		if (zms[i].used) {
			//IMAGE* img = &imgZM[zms[i].flameIndex];
			//IMAGE* img = zms[i].dead ? imgZMDead : imgZM;
			IMAGE* img = NULL;
			if (zms[i].dead) {
				img = imgZMDead;
			}
			else if (zms[i].eating) {
				img = imgZMEat;
			}
			else {
				img = imgZM;
			}
			img += zms[i].flameIndex;
			putimagePNG(zms[i].x, zms[i].y - img->getheight(), img);
		}
	}
}

void drawSunShines() {
	//渲染掉落的阳光球
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++) {
		//if (balls[i].used || balls[i].xoff) {
		if (balls[i].used) {
			IMAGE* img = &imgSunshineBall[balls[i].flameIndex];
			//putimagePNG(balls[i].x, balls[i].y, img);
			putimagePNG(balls[i].pCur.x, balls[i].pCur.y, img);
		}
	}

	char scoreText[8];
	sprintf_s(scoreText, sizeof(scoreText), "%d", sunShine);
	outtextxy(272, 55, scoreText);  //输出分数
}

void drawCards() {
	//渲染植物卡牌
	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		int x = 325 + i * 55;
		int y = 6;
		putimage(x, y, &imgCards[i]);
	}
}

void drawZhiWu() {
	//渲染种植下去的植物
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				//int x = 250 + j * 81 + 9;
				//int y = 84 + i * 97 + 12;
				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].flameIndex;
				//putimagePNG(x, y, imgZhiWu[zhiWuType][index]);
				putimagePNG(map[i][j].x, map[i][j].y, imgZhiWu[zhiWuType][index]);
			}
		}
	}

	//渲染移动过程中的植物
	if (curZhiWu > 0) {
		IMAGE* img = imgZhiWu[curZhiWu - 1][0];
		putimagePNG(curX - img->getwidth() / 2, curY - img->getheight() / 2, img);
	}
}

void drawBullets() {
	//渲染豌豆子弹
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < bulletMax; i++) {
		if (bullets[i].used) {
			if (bullets[i].blast) {
				IMAGE* img = &imgBulletBlast[bullets[i].flameIndex];
				putimagePNG(bullets[i].x, bullets[i].y, img);
			}
			else {
				putimagePNG(bullets[i].x, bullets[i].y, &imgBulletNormal);
			}
		}
	}
}

void updateWindow(){
	BeginBatchDraw();  //开始缓冲

	putimage(0, 0, &imgBg);
	//putimage(250, 0, &imgBar);
	putimagePNG(250, 0, &imgBar);

	drawCards();
	drawZhiWu();
	drawZM();  //渲染僵尸
	drawSunShines();  //渲染阳光球
	drawBullets();
	
	EndBatchDraw();  //结束双缓冲
}

//收集阳光
void collectSunshine(ExMessage* msg) {
	int count = sizeof(balls) / sizeof(balls[0]);
	int w = imgSunshineBall[0].getwidth();
	int h = imgSunshineBall[0].getheight();
	for (int i = 0; i < count; i++) {
		if (balls[i].used) {
			//int x = balls[i].x;
			//int y = balls[i].y;
			int x = balls[i].pCur.x;
			int y = balls[i].pCur.y;
			if (msg->x > x && msg->x<x + w && msg->y>y && msg->y < y + h) {
				//balls[i].used = false;
				balls[i].status = SUNSHINE_COLLECT;
				//sunShine += 25;
				//mciSendString("play ../植物大战僵尸-素材/res/sunshine.mp3", 0, 0, 0);
				PlaySound("../植物大战僵尸-素材/res/sunshine.wav", NULL, SND_FILENAME | SND_ASYNC);
				//设置阳光球偏移量
				//float destX = 250;
				//float destY = 0;
				//float angle = atan((balls[i].y - destY) / (balls[i].x - destX));
				//balls[i].yoff = 6 * sin(angle);  //4
				//balls[i].xoff = 6 * cos(angle);  //4
				balls[i].p1 = balls[i].pCur;
				balls[i].p4 = vector2(250, 0);
				balls[i].t = 0;
				float distance = dis(balls[i].p1 - balls[i].p4);
				float off = 8;
				balls[i].speed = 1.0 / (distance / off);
				break;
			}
		}
	}
}

//种植植物的阳光消耗的判断
boolean zhongZhi(int type) {
	if (type == 1) {
		if (sunShine >= 100) {
			sunShine -= 100;
			return true;
		}
		else {
			return false;
		}
	}
	if (type == 2) {
		if (sunShine >= 50) {
			sunShine -= 50;
			return true;
		}
		else {
			return false;
		}
	}
}

//判断用户点击
void userClick() {
	ExMessage msg;
	static int status = 0;
	if (peekmessage(&msg)) {
		if (msg.message == WM_LBUTTONDOWN) {
			if (msg.x > 325 && msg.x < 325 + 55 * ZHI_WU_COUNT && msg.y < 80 && msg.y > 5) {
				int index = (msg.x - 325) / 55;
				status = 1;
				curZhiWu = index + 1;
			}
			else {
				collectSunshine(&msg);  
			}
		}
		else if (msg.message == WM_MOUSEMOVE && status == 1) {
			curX = msg.x;
			curY = msg.y;
		}
		else if (msg.message == WM_LBUTTONUP && status == 1) {
			if (msg.x > 250 && msg.x < 982 && msg.y>84 && msg.y < 573) {
				int row = (msg.y - 84) / 97;
				int col = (msg.x - 250) / 81;
				if (map[row][col].type == 0) {
					map[row][col].type = curZhiWu;

					//种植植物的阳光消耗
					if (zhongZhi(map[row][col].type)) {
						map[row][col].flameIndex = 0;
						map[row][col].shootTime = 0;
						//int x = 250 + j * 81 + 9;
						//int y = 84 + i * 97 + 12;
						map[row][col].x = 250 + col * 81 + 9;
						map[row][col].y = 84 + row * 97 + 12;
					}
					else {
						map[row][col].type = 0;
					}
				}
			}
			curZhiWu = 0;
			status = 0;
		}
	}
}

void createSunshine() {
	static int count = 0;
	static int fre = 600;  //400
	count++;
	if (count >= fre) {
		fre = 400 + rand() % 200;
		count = 0;

		//从阳光池中取出一个可以使用的
		int ballMax = sizeof(balls) / sizeof(balls[0]);

		int i;
		for (i = 0; i < ballMax && balls[i].used; i++);
		if (i >= ballMax) {
			return;
		}

		balls[i].flameIndex = 0;
		balls[i].used = true;
		//balls[i].x = 246 + rand() % (967 - 246);  //246-967
		//balls[i].y = 75;
		//balls[i].destY = 84 + (rand() % 6) * 97;
		balls[i].timer = 0;
		//balls[i].xoff = 0;
		//balls[i].yoff = 0;

		balls[i].status = SUNSHINE_DOWN;
		balls[i].t = 0;
		balls[i].p1 = vector2(246 + rand() % (967 - 246), 75);
		balls[i].p4 = vector2(balls[i].p1.x, 84 + (rand() % 6) * 97);
		int off = 1;  //2
		float distance = dis(balls[i].p1.y - balls[i].p4.y);
		balls[i].speed = 1.0 / (distance / off);
	}

	//向日葵生产阳光
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == XIANG_RI_KUI + 1) {
				map[i][j].timer++;
				if (map[i][j].timer > 600) {  //200
					map[i][j].timer = 0;

					int k;
					for (k = 0; k < ballMax && balls[k].used; k++);
					if (k >= ballMax) {
						return;
					}

					balls[k].used = true;
					balls[k].status = SUNSHINE_PRODUCT;
					balls[k].p1 = vector2(map[i][j].x, map[i][j].y);
					int width = ((40 + rand() % 50) * (rand() % 2 ? 1 : -1));
					balls[k].p4 = vector2(map[i][j].x + width,
						map[i][j].y + imgZhiWu[XIANG_RI_KUI][0]->getheight() -
						imgSunshineBall[0].getheight());
					balls[k].p2 = vector2(balls[k].p1.x + width * 0.3, balls[k].p1.y - 100);
					balls[k].p3 = vector2(balls[k].p1.x + width * 0.7, balls[k].p1.y - 50);
					balls[k].t = 0;
					balls[k].speed = 0.05;
				}
			}
		}
	}
}

void updateSunshine() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++) {
		if (balls[i].used) {
			balls[i].flameIndex = (balls[i].flameIndex + 1) % 29;
			if (balls[i].status == SUNSHINE_DOWN) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);  //p1+t*(p4-p1);
				if (sun->t >= 1) {
					sun->timer = 0;
					sun->status = SUNSHINE_GROUND;
				}
			}
			else if (balls[i].status == SUNSHINE_GROUND) {
				balls[i].timer++;
				if (balls[i].timer > 300) {  //100
					balls[i].used = false;
					balls[i].timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_COLLECT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t > 1) {
					sun->used = false;
					sunShine += 25;
				}
			}
			else if (balls[i].status == SUNSHINE_PRODUCT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = calcBezierPoint(sun->t, sun->p1, sun->p2, sun->p3, sun->p4);
				if (sun->t > 1) {
					sun->status = SUNSHINE_GROUND;
					sun->t = 0;
				}
			}
			//balls[i].flameIndex = (balls[i].flameIndex + 1) % 29;
			//if (balls[i].timer == 0) {
			//	balls[i].y += 3;
			//}
			//if (balls[i].y >= balls[i].destY) {
			//	balls[i].timer++;
			//	if (balls[i].timer > 100) {
			//		balls[i].used = false;
			//	}
			//}
		}
		//else if (balls[i].xoff) {
		//	float destX = 250;
		//	float destY = 0;
		//	float angle = atan((balls[i].y - destY) / (balls[i].x - destX));
		//	balls[i].yoff = 6 * sin(angle);  //4
		//	balls[i].xoff = 6 * cos(angle);  //4

		//	balls[i].x -= balls[i].xoff;
		//	balls[i].y -= balls[i].yoff;
		//	if (balls[i].y < 0 || balls[i].x < 250) {
		//		balls[i].xoff = 0;
		//		balls[i].yoff = 0;
		//		sunShine += 25;
		//	}
		//}
	}
}

void createZM() {
	/*static int count3 = 0;
	if (++count3 < 3) {
		return;
	}
	count3 = 0;*/
	if (zmCount >= ZM_MAX) {
		return;
	}

	static int count = 0;
	static int zmFre = 250;  //200
	count++;
	if (count > zmFre) {
		count = 0;
		zmFre = rand() % 500 + 500;  //200

		int i;
		int zmMAX = sizeof(zms) / sizeof(zms[0]);
		for (i = 0; i < zmMAX && zms[i].used; i++);
		if (i < zmMAX) {
			memset(&zms[i], 0, sizeof(zms[i]));
			zms[i].used = true;
			zms[i].x = WIN_WIDTH;
			zms[i].row = rand() % 5;
			zms[i].y = 84 + (1 + zms[i].row) * 97;
			zms[i].speed = 1;
			zms[i].blood = 100;
			zms[i].dead = false;
			zmCount++;
		}
	}
}

void updateZM() {
	int zmMAX = sizeof(zms) / sizeof(zms[0]);

	static int count = 0;
	count++;
	if (count > 7) {
		count = 0;
		//更新僵尸位置
		for (int i = 0; i < zmMAX; i++) {
			if (zms[i].used) {
				zms[i].x -= zms[i].speed;
				if (zms[i].x < 170) {
					//printf("game over\n");
					//MessageBox(NULL, "over", "over", 0);  //待优化
					//exit(0);  //待优化
					gameStatus = FAIL;
				}
			}
		}
	}

	static int count2 = 0;
	count2++;
	if (count2 > 4) {
		count2 = 0;
		//更改僵尸的序列帧
		for (int i = 0; i < zmMAX; i++) {
			if (zms[i].used) {
				if (zms[i].dead) {
					zms[i].flameIndex++;
					if (zms[i].flameIndex >= 20) {
						zms[i].used = false;
						killCount++;
						if (killCount == ZM_MAX) {
							gameStatus = WIN;
						}
					}
				}
				else if (zms[i].eating) {
					zms[i].flameIndex = (zms[i].flameIndex + 1) % 21;
				}
				else {
					zms[i].flameIndex = (zms[i].flameIndex + 1) % 22;
				}
			}
		}
	}
}

void shoot() {
	static int count = 0;
	if (++count < 6) {
		return;
	}
	count = 0;

	int lines[5] = { 0 };
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	int dangerX = WIN_WIDTH - imgZM[0].getwidth();
	for (int i = 0; i < zmCount; i++) {
		if (zms[i].used && zms[i].x < dangerX) {
			lines[zms[i].row] = 1;
		}
	}

	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == WAN_DOU + 1 && lines[i]) {
				//static int count = 0;
				//count++;
				map[i][j].shootTime++;
				//if (count > 20) {
					//count = 0;
				if (map[i][j].shootTime > 20) {
					map[i][j].shootTime = 0;

					int k;
					for (k = 0; k < bulletMax && bullets[k].used; k++);
					if (k < bulletMax) {
						bullets[k].used = true;
						bullets[k].row = i;
						bullets[k].speed = 5;	//6

						bullets[k].blast = false;
						bullets[k].flameIndex = 0;

						int zwX = 250 + j * 81 + 9;
						int zwY = 84 + i * 97 + 12;
						bullets[k].x = zwX + imgZhiWu[map[i][j].type - 1][0]->getwidth() - 10;
						bullets[k].y = zwY + 5;
					}
				}
			}
		}
	}
}

void updateBullets() {
	static int count = 0;
	if (++count < 2) {
		return;
	}
	count = 0;

	int countMax = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < countMax; i++) {
		if (bullets[i].used) {
			bullets[i].x += bullets[i].speed;
			if (bullets[i].x > WIN_WIDTH) {
				bullets[i].used = false;
			}

			if (bullets[i].blast) {
				bullets[i].flameIndex++;
				if (bullets[i].flameIndex >= 4) {
					bullets[i].used = false;
				}
			}
		}
	}
}

void checkBullet2Zm() {
	int bCount = sizeof(bullets) / sizeof(bullets[0]);
	int zCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < bCount; i++) {
		if (bullets[i].used == false || bullets[i].blast == true) {
			continue;
		}
		for (int j = 0; j < zCount; j++) {
			if (zms[j].used == false) {
				continue;
			}
			int x1 = zms[j].x + 80;
			int x2 = zms[j].x + 110;
			int x = bullets[i].x;
			if (zms[j].dead == false && bullets[i].row == zms[j].row && x > x1 && x < x2) {
				bullets[i].speed = 0;
				bullets[i].blast = true;
				zms[j].blood -= 10;

				if (zms[j].blood <= 0) {
					zms[j].dead = true;
					zms[j].flameIndex = 0;
					zms[j].speed = 0;
				}
				break;
			}
		}
	}
}

void checkZm2ZhiWu() {
	int zCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zCount; i++) {
		if (zms[i].dead) {
			continue;
		}

		int row = zms[i].row;
		for (int k = 0; k < 9; k++) {
			if (map[row][k].type == 0) {
				continue;
			}

			int zhiWuX = 250 + k * 81 + 9;
			int x1 = zhiWuX + 10;
			int x2 = zhiWuX + 60;
			int x3 = zms[i].x + 80;
			if (x3 > x1 && x3 < x2) {
				if (map[row][k].catched) {
					map[row][k].deadTime++;
					if (map[row][k].deadTime > 180) {  //100
						map[row][k].deadTime = 0;
						map[row][k].type = 0;
						zms[i].eating = false;
						zms[i].flameIndex = 0;
						zms[i].speed = 1;
					}
				}
				else {
					map[row][k].catched = true;
					map[row][k].deadTime = 0;
					zms[i].eating = true;
					zms[i].speed = 0;
					zms[i].flameIndex = 0;
				}
			}
		}
	}
}

void collisionCheck() {
	checkBullet2Zm();  //子弹对僵尸的碰撞检测

	checkZm2ZhiWu();  //僵尸对植物的碰撞检测
}

void updateZhiWu() {
	static int count = 0;
	if (++count < 3) {
		return;
	}
	count = 0;

	//实现植物的摆动（改变植物图片的索引帧）
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				map[i][j].flameIndex++;
				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].flameIndex;
				if (imgZhiWu[zhiWuType][index] == NULL) {
					map[i][j].flameIndex = 0;
				}
			}
		}
	}
}

//更新游戏数据
void updateGame() {
	updateZhiWu();//实现植物的摆动

	createSunshine();  //创建阳光球
	updateSunshine();  //更新阳光球状态

	createZM();  //创建僵尸
	updateZM();  //更新僵尸状态

	shoot();  //发射豌豆子弹
	updateBullets();  //更新豌豆子弹

	collisionCheck();  //碰撞检测
}

//游戏开始界面
void startUI() {
	IMAGE imgBg, imgMenu1, imgMenu2;
	loadimage(&imgBg,"../植物大战僵尸-素材/res/Screen/MainMenu.png");
	loadimage(&imgMenu1, "../植物大战僵尸-素材/res/Screen/Adventure_1.png");
	loadimage(&imgMenu2, "../植物大战僵尸-素材/res/Screen/Adventure_0.png");

	int flag = 0;
	while(1) {
		BeginBatchDraw();

		putimage(0, 0, &imgBg);
		putimagePNG(496, 108, flag ? &imgMenu2 : &imgMenu1);

		ExMessage msg;
		if (peekmessage(&msg)) {
			if (msg.message == WM_LBUTTONDOWN && msg.x > 496 && msg.x < 496 + 255 &&
				msg.y>105 && msg.y < 108 + 98) {
				flag = 1;
			}
			else if (msg.message == WM_LBUTTONUP && flag) {
				EndBatchDraw();
				break;
			}
		}
		EndBatchDraw();
	}
}

void viewScene() {
	int xMin = WIN_WIDTH - imgBg.getwidth();  //1000-1400
	vector2 points[9] = {
		/*{550,80},{530,160},{630,170},{530,200},{515,270},{565,370},{605,340},
		{705,280},{690,340}*/
		{750,80},{690,160},{790,220},{730,200},{715,270},{650,370},{680,340},
		{785,280},{770,340}
	};
	int index[9];
	for (int i = 0; i < 9; i++) {
		index[i] = rand() % 12;
	}

	int count = 0;
	for (int i = 0; i >= xMin; i -= 2) {
		BeginBatchDraw();
		putimage(i, 0, &imgBg);

		count++;
		for (int k = 0; k < 9; k++) {
			putimagePNG(points[k].x - xMin + i, points[k].y, &imgZMStand[index[k]]);
			if (count > 10) {
				index[k] = (index[k] + 1) % 11;
			}
		}
		if (count > 10) {
			count = 0;
		}
		EndBatchDraw();
		Sleep(5);
	}

	//停留时间
	for (int i = 0; i < 100; i++) {
		BeginBatchDraw();  // 启动绘图批处理模式
		
		putimage(xMin, 0, &imgBg);
		for (int k = 0; k < 9; k++) {
			putimagePNG(points[k].x, points[k].y, &imgZMStand[index[k]]);
			index[k] = (index[k] + 1) % 11;
		}
		Sleep(30);
		EndBatchDraw();   // 结束绘图批处理模式，将缓存的绘图操作一次性绘制到屏幕上
	}

	//拉回
	for (int i = xMin; i < 0; i += 2) {
		BeginBatchDraw();

		putimage(i, 0, &imgBg);

		count++;
		for (int k = 0; k < 9; k++) {
			putimagePNG(points[k].x - xMin + i, points[k].y, &imgZMStand[index[k]]);
			if (count >= 10) {
				index[k] = (index[k] + 1) % 11;
			}
		}
		if (count >= 10) {
			count = 0;
		}

		EndBatchDraw();
		Sleep(5);

		mciSendString("play ../植物大战僵尸-素材/res/audio/awooga.mp3", 0, 0, 0);
	}
}

void barsDown() {
	int height = imgBar.getheight();
	for (int y = - height; y < 0; y+=2) {
		BeginBatchDraw();

		putimage(0, 0, &imgBg);
		putimagePNG(250, y, &imgBar);

		for (int i = 0; i < ZHI_WU_COUNT; i++) {
			int x = 325 + i * 55;
			putimage(x, 6+y, &imgCards[i]);
		}

		EndBatchDraw();
		Sleep(10);
	}
}

bool checkOver() {
	int ret = false;
	if (gameStatus == WIN) {
		mciSendString("close ../植物大战僵尸-素材/res/bg.MP3", NULL, 0, NULL);
		Sleep(2000);
		mciSendString("play ../植物大战僵尸-素材/res/win.mp3", 0, 0, 0);
		loadimage(0, "../植物大战僵尸-素材/res/gameWin.png");
		ret = true;
	}
	else if (gameStatus == FAIL) {
		mciSendString("close ../植物大战僵尸-素材/res/bg.MP3", NULL, 0, NULL);
		Sleep(2000);
		loadimage(0, "../植物大战僵尸-素材/res/gameFail.png");
		mciSendString("play ../植物大战僵尸-素材/res/lose.mp3", 0, 0, 0);
		ret = true;
	}
	else if (gameStatus == GOING) {
		mciSendString("play ../植物大战僵尸-素材/res/bg.MP3 repeat", 0, 0, 0);
		ret = false;
	}

	return ret;
}

int main(void) {
	gameInit();  //初始化游戏数据

	startUI();  //游戏界面

	viewScene();  //片头巡场

	barsDown();  //降下工具栏

	int timer = 0;
	bool flag = true;
	while (1) {
		userClick();

		timer += getDelay();
		if (timer > 10) {  //35
			flag = true;
			timer = 0;
		}
		if (flag) {
			flag = false;
			updateWindow();
			updateGame();
			if (checkOver()) {
				break;
			}
		}
	}
	
	system("pause");
	return 0;
}