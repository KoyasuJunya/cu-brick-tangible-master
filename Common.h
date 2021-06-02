#pragma once
#define TATE			9
#define YOKO			7
#define SUB				3
#define CHANNEL		1

typedef struct{
	uint8_t i, j, type;

	void SetLabel(uint8_t i, uint8_t j, uint8_t type){
		this->i = i;
		this->j = j;
		this->type = type;
	}
} Label;

MicroBitImage smile("0,1,0,1,0\n0,1,0,1,0\n0,0,0,0,0\n1,0,0,0,1\n0,1,1,1,0\n"), //ニコちゃんマーク
				  sad("0,0,0,0,0\n1,1,0,1,1\n0,0,0,0,1\n0,1,1,1,0\n1,0,0,0,1\n"), //悲しいマーク
				  heart("0,1,0,1,0\n1,1,1,1,1\n1,1,1,1,1\n0,1,1,1,0\n0,0,1,0,0\n"), //ハートマーク
				  smallHeart("0,0,0,0,0\n0,1,0,1,0\n0,1,1,1,0\n0,0,1,0,0\n0,0,0,0,0\n"), //小さいハートマーク
				  square("1,1,1,1,1\n1,0,0,0,1\n1,0,0,0,1\n1,0,0,0,1\n1,1,1,1,1\n"), //□
				  fillSquare("0,0,0,0,0\n0,1,1,1,0\n0,1,1,1,0\n0,1,1,1,0\n0,0,0,0,0\n"), //■
				  circle("0,1,1,1,0\n1,0,0,0,1\n1,0,0,0,1\n1,0,0,0,1\n0,1,1,1,0\n"), //丸印
				  cross("1,0,0,0,1\n0,1,0,1,0\n0,0,1,0,0\n0,1,0,1,0\n1,0,0,0,1\n"); //バツ印
