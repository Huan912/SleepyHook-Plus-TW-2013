#include "..//Main.h"
#include "..//Utils.h"
#include "PacketFriend.h"

#define FRIEND_OPCODE 200;
CFriendPacket g_FriendPacket;

enum EFriendSubId
{
	SUB_LIST = 0,
	SUB_UPDATE = 1,
	SUB_REQUEST = 2,
	SUB_ACTION_RESULT = 3,
};

int CFriendPacket::GetOpcode() const
{
	return FRIEND_OPCODE;
}

void CFriendPacket::OnReceive(void* pBuffer, int nLength)
{
	CPacketReader pkt(pBuffer, nLength);
	int iSubId = pkt.Read<unsigned char>();

	// TODO: Implement Call the game function to add a friend cuz NMCO bypass broke the friends system (Due to some issues, I am not release full fix code) 
	//       This is example how to add a packet
	// TODO: 嘗試研究並調用遊戲函數來新增好友，因為NMCO繞過的關係，好友系統被破壞了，只能靠我們自己實現 (由於一些問題我暫時並不開源完整修復方法)
	//       這裡只示範如何添加一個自訂的新封包

	switch (iSubId)
	{
	case SUB_LIST:
	case SUB_UPDATE:
	case SUB_REQUEST:
	case SUB_ACTION_RESULT:
		break;
	default:
		break;
	}
}
