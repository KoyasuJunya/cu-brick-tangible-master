//データ受信 インタープリタ

#include "MicroBit.h"
#include "Common.h"
#define NORMAL_STEP	1
#define BRANCH_STEP	2
#define TYPE_BASIC	0
#define TYPE_LABEL	1
#define TYPE_JUMP		2
#define TYPE_BRANCH	3
//自前で用意したボタンインスタンスのID
#define ID_BUTTON_P1	10
#define ID_BUTTON_P2	11

//Play()で鳴らせる音の種類
typedef enum{
	A = 220,
	B = 247,
	C = 262,
	D = 294,
	E = 330,
	F = 350,
	G = 392
} key;

//Labelに状態を表す変数を追加したもの
typedef struct{
	Label label;
	bool stat = false;

	void SetExLabel(uint8_t i, uint8_t j, uint8_t type, bool stat){
		this->label.SetLabel(i, j, type);
		this->stat = stat;
	}
} ExLabel;

//現在位置とボード全体の識別番号を管理するもの
typedef struct{
	uint8_t i, j, board[TATE][YOKO];

	//ExLabelから現在位置を設定
	void SetPosition(ExLabel exLabel){
		this->i = exLabel.label.i;
		this->j = exLabel.label.j;
	}

	//1行分の識別番号をセットする
	void SetBlockNum(int line, PacketBuffer buf){
		for(int i = 0; i < YOKO; i++) this->board[line][i] = buf[i + 1];
	}

	//現在位置の識別番号を返す
	uint8_t GetBlockNum(){
		return this->board[this->i][this->j];
	}

	//右にstep進めるかチェック
	bool CheckRight(int step){
		return this->j + step < YOKO && this->board[this->i][this->j + step] != 0;
	}

	//下にstep進めるかチェック
	bool CheckBottom(int step){
		return this->i + step < TATE && this->board[this->i + step][this->j] != 0;
	}

	//現在位置から次の位置へ進めるなら進む
	bool GoNext(){
		bool hasRight = this->CheckRight(NORMAL_STEP);
		bool hasBottom = this->CheckBottom(NORMAL_STEP);
		//右,下どちらにもブロックがある,またはどちらにもない場合はfalse
		//進めるならその方向に進んでtrueを返す
		if(!(hasRight xor hasBottom)) return false;
		if(hasRight){
			this->j += NORMAL_STEP;
		}else{
			this->i += NORMAL_STEP;
		}
		return true;
	}

	//現在位置から次の位置へ進めるなら進む(ifの場合)
	bool GoNext(bool dir){
		//分岐の場合は進む方向だけチェック
		if(dir){
			if(!this->CheckRight(BRANCH_STEP)) return false;
			this->j += BRANCH_STEP;
		}else{
			if(!this->CheckBottom(BRANCH_STEP)) return false;
			this->i += BRANCH_STEP;
		}
		return true;
	}
} Position;

MicroBit uBit;
bool hitFlag; //的に当たったフラグ
bool running, programReady, exceptionFlag; //実行フラグ,実行可能フラグ,例外フラグ
Position currentPosition; //現在位置
ExLabel labels[SUB + 1]; //ラベルブロックの座標情報
//任意のピンからボタンインスタンスを作成
MicroBitButton pin1Button(MICROBIT_PIN_P1, ID_BUTTON_P1, MICROBIT_BUTTON_ALL_EVENTS, PullUp),
				   pin2Button(MICROBIT_PIN_P2, ID_BUTTON_P2, MICROBIT_BUTTON_ALL_EVENTS, PullUp);

//音を鳴らす関数
void Play(key key, uint8_t oct, int ms){
   if(oct > 4) return;
   int wait = 1000/*000*/ / (key * oct) / 2;
   long int time = ms / 2 * 1000 / wait;
   for(long int i = 0; i < time; i++){
      uBit.io.P0.setDigitalValue(1);
      uBit.sleep(wait);//修正前wait_us(wait);
      uBit.io.P0.setDigitalValue(0);
      uBit.sleep(wait);//修正前wait_us(wait);  //uBit.sleep(ミリ秒);//wait_us(マイクロ);
   }
}

//ifフラグの初期化
void InitIfFlags(){
	hitFlag = false;
	//ifフラグを使う場合に適宜追記
}

//座標情報などは初期化せずプログラムを再度実行可能にする
void Restart(){
   uBit.display.clear();
   running = false;
}

//ボタンが押された際にボタンごとの動作を実行する
void OnButton(MicroBitEvent e){
	if(!programReady) return;
   switch(e.source){
      case MICROBIT_ID_BUTTON_A:
			if(running) return;
			currentPosition.SetPosition(labels[0]);
			InitIfFlags();
			uBit.display.clear();
			running = true;
         break;
      case MICROBIT_ID_BUTTON_AB:
         Restart();
			break;
		case ID_BUTTON_P1:
		case ID_BUTTON_P2:
			hitFlag = true;
   }
}

//変数やフラグ,座標情報などを初期化する
void Reset(){
   uBit.display.clear();
	programReady = false;
   running = false;
   for(int i = 0; i <= SUB; i++) labels[i].stat = false;
}

//プログラムを実行可能状態にする
void Ready(){
	exceptionFlag = false;
	programReady = true;
}
	
//通信の同期,受信したデータのチェックを行う
bool WaitAndReceive(uint8_t receiveStat, PacketBuffer &buf){
	//最初の受信であるReset信号はresponseを送信しない
	//それ以外はresponseを送信して受信を待つ
	if(receiveStat != 1){
		PacketBuffer response(1);
		response[0] = receiveStat;
		uBit.radio.datagram.send(response);
	}
	uint8_t wait = 0;
	while((buf = uBit.radio.datagram.recv()) == PacketBuffer::EmptyPacket){
		uBit.sleep(20);
		wait++;
		if(wait > 200) break;
	}
	if(buf == PacketBuffer::EmptyPacket || buf[0] != receiveStat){
		uBit.display.printAsync(sad);
		return false;
	}
	return true;
}

//必要なデータを順番に受信する(データを受信した際に実行され、一連の受信が終わるまでブロックする)
void DataReceive(MicroBitEvent){
	PacketBuffer buf;
	//Reset信号受信
	if(!WaitAndReceive(1, buf)) return;
	Reset();
	//識別番号受信
	for(int i = 0; i < TATE; i++){
		if(!WaitAndReceive(2, buf)) return;
		currentPosition.SetBlockNum(i, buf);
	}
	//座標情報の数受信
	if(!WaitAndReceive(3, buf) || buf[1] > SUB + 1 || !buf[1]) return;
	uint8_t numOfUsedLabel = buf[1];
	//座標情報受信
	for(int i = 0; i < numOfUsedLabel; i++){
		if(!WaitAndReceive(4, buf)) return;
		labels[buf[3]].SetExLabel(buf[1], buf[2], buf[3], true);
	}
	//Ready信号受信
	if(!WaitAndReceive(5, buf)) return;
	Ready();
}

//イメージの表示(任意の時間)
void Print(MicroBitImage image, int ms){
	uBit.display.print(image);
	uBit.sleep(ms);//修正前wait_ms(ms);
	uBit.display.clear();
}

//通常ブロックの動作
void Exec(uint8_t order){
   switch(order){
      case 1:
			//ハートを表示
         Print(heart, 600);
         break;
      case 2:
			//小さいハートを表示
         Print(smallHeart, 600);
         break;
      case 3:
			//丸印を表示
         Print(circle, 600);
         break;
		case 4:
			//バツ印を表示
			Print(cross, 600);
			break;
		case 7:
			//高い音
			Play(C, 2, 600);
			break;
		case 8:
			//低い音
			Play(C, 1, 600);
			break;
      default:
         exceptionFlag = true;
   }
}

//ifの条件を判定する
bool Eval(uint8_t evalItem){
   switch(evalItem){
      case 1:
			//的を倒した？
         return hitFlag;
		case 2:
			//的を全部倒した？
			return !pin1Button.isPressed() && !pin2Button.isPressed();
   }
   exceptionFlag = true;
   return false;
}

//識別番号に対応した動作を実行した後,現在位置を更新する
void Decode(uint8_t blockNum){
	bool result;
	uint8_t serial = blockNum & 0b00011111; //通し番号
	switch(blockNum >> 6){
		case TYPE_BASIC:
			Exec(serial);
		case TYPE_LABEL:
			if(exceptionFlag || !currentPosition.GoNext()) break;
			return;
		case TYPE_JUMP:
			if(serial < 1 || serial > SUB || !labels[serial].stat){
				exceptionFlag = true;
				break;
			}else{
				currentPosition.SetPosition(labels[serial]);
			}
			return;
		case TYPE_BRANCH:
			result = Eval(serial);
			if(exceptionFlag || !currentPosition.GoNext(result)) break;
			InitIfFlags();
			return;
	}
	//不正な識別番号や未定義ラベルへのジャンプが
	//実行された場合は例外として異常終了する
	if(exceptionFlag){
		Reset();
		uBit.display.printAsync(sad);
		return;
	}else{
		Restart();
		return;
	}
}

//プログラム実行ルーチン
void Start(){
	while(true){
		uBit.sleep(10);
		if(running){
			Decode(currentPosition.GetBlockNum());
		}else{
			if(programReady) uBit.display.printAsync(smile);
			return;
		}
	}
}

//イベントハンドラを有効にする
void ListenEvt(){
   uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, DataReceive, MESSAGE_BUS_LISTENER_DROP_IF_BUSY);
   uBit.messageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_BUTTON_EVT_CLICK, OnButton, MESSAGE_BUS_LISTENER_DROP_IF_BUSY);
   uBit.messageBus.listen(MICROBIT_ID_BUTTON_AB, MICROBIT_BUTTON_EVT_HOLD, OnButton, MESSAGE_BUS_LISTENER_DROP_IF_BUSY);
	uBit.messageBus.listen(ID_BUTTON_P1, MICROBIT_BUTTON_EVT_UP, OnButton, MESSAGE_BUS_LISTENER_DROP_IF_BUSY);
	uBit.messageBus.listen(ID_BUTTON_P2, MICROBIT_BUTTON_EVT_UP, OnButton, MESSAGE_BUS_LISTENER_DROP_IF_BUSY);
}


int main(){
   uBit.init();
	ListenEvt();
   uBit.radio.setGroup(CHANNEL); //チャンネルを設定
   uBit.radio.enable(); //無線を有効に
   //データ受信待ち
   while(true){
      uBit.sleep(100);
      Start();
   }
}
