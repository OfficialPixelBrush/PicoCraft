#include <WiFi.h>  // Use WiFiNINA if you installed that library

// Replace with your network credentials
#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

WiFiServer server(25565); // Using port 25565 for Minecraft server
#define MAX_CONNECTIONS 4

// Helper Stuff 
  enum Packet {
    KeepAlive           = 0x00,
    LoginRequest        = 0x01,
    Handshake           = 0x02,
    ChatMessage         = 0x03,
    TimeUpdate          = 0x04,
    EntityEquipment     = 0x05,
    SpawnPosition       = 0x06,
    UseEntity           = 0x07,
    UpdateHealth        = 0x08,
    Respawn             = 0x09,
    Player              = 0x0A,
    PlayerPosition      = 0x0B,
    PlayerLook          = 0x0C,
    PlayerPositionLook  = 0x0D,
    PlayerDigging       = 0x0E,
    PlayerBlockPlacement= 0x0F,
    HoldingChange       = 0x10,
    UseBed              = 0x11,
    Animation           = 0x12,
    EntityAction        = 0x13,
    NamedEntitySpawn    = 0x14,
    PickupSpawn         = 0x15,
    CollectItem         = 0x16,
    AddObjectVehicle    = 0x17,
    MobSpawn            = 0x18,
    EntityPainting      = 0x19,
    StanceUpdate        = 0x1B,
    EntityVelocity      = 0x1C,
    DestroyEntity       = 0x1D,
    Entity              = 0x1E,
    EntityRelativeMove  = 0x1F,
    EntityLook          = 0x20,
    EntityLookRelativeMove  = 0x21,
    EntityTeleport      = 0x22,
    EntityStatus        = 0x26,
    AttachEntity        = 0x27,
    EntityMetadata      = 0x28,
    PreChunk            = 0x32,
    Chunk               = 0x33,
    MultiBlockChange    = 0x34,
    BlockChange         = 0x35,
    BlockAction         = 0x36,
    Explosion           = 0x3C,
    Soundeffect         = 0x3D,
    NewInvalidState     = 0x46,
    Thunderbolt         = 0x47,
    OpenWindow          = 0x64,
    CloseWindow         = 0x65,
    WindowClick         = 0x66,
    SetSlot             = 0x67,
    WindowItems         = 0x68,
    UpdateProgressBar   = 0x69,
    Transaction         = 0x6A,
    UpdateSign          = 0x82,
    MapData             = 0x83,
    IncrementStatistic  = 0xC8,
    Disconnect          = 0xFF
  };

  struct Vec3 {
    double x = 0.0, y = 0.0, z = 0.0;
  };

  struct Int3 {
    int x = 0, y = 0, z = 0;
  };

  struct Int3 spawnPoint = {
    8,
    64,
    8
  };

  void FaceOffset(Int3& pos, int8_t face) {
    switch(face) {
      case 0:
        pos.y--;
        break;
      case 1:
        pos.y++;
        break;
      case 2:
        pos.z--;
        break;
      case 3:
        pos.z++;
        break;
      case 4:
        pos.x--;
        break;
      case 5:
        pos.x++;
        break;
    }
  }


#define INVENTORY_HOTBAR 36

enum ConnectionProgress {
  Disconnected,
  TryingToConnect,
  Shake,
  Login,
  Connected
};

struct Player {
  int32_t entityId;
  char* username = nullptr;
  Vec3 position;
  float yaw = 0.0;
  float pitch = 0.0;
  double stance = 1.5;
  bool onGround = false;  
  int8_t hotbarSlot = 0;
  int32_t leftToLoad = 1;
  int8_t connectionStage = Disconnected;
  bool hasSentPacket = false;
  WiFiClient client;
};

struct Player players[MAX_CONNECTIONS];
int32_t globalEntityId = 0;

static const char compressedChunk[] PROGMEM  = {
  120,218,237,205,161,13,0,32,16,4,65,2,226,37,253,119,73,7,208,1,136,51,136,25,191,217,26,153,222,50,229,239,
  239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,
  239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,
  239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,
  239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,255,193,31,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,120,219,
  119,75,175,215,235,245,122,189,94,175,215,235,245,122,189,94,175,215,235,245,122,189,94,175,215,235,245,122,189,94,175,215,
  235,245,122,189,94,175,215,235,245,122,189,94,175,215,235,245,122,189,94,175,215,235,245,122,125,220,207,3,138,218,152,0
};

// Basic communication
int8_t ReadByte(WiFiClient& client) {
  return (int8_t)client.read();
}

void WriteByte(WiFiClient& client, int8_t value) {
  client.write(value);
}

int16_t ReadShort(WiFiClient& client) {
  int16_t byte1 = client.read();
  int16_t byte2 = client.read();
  return (byte1 << 8 | byte2);
}

void WriteShort(WiFiClient& client, int16_t value) {
  uint8_t byte1 = (value >> 8 ) & 0xFF;
  uint8_t byte0 = (value 		) & 0xFF;
  client.write(byte1);
  client.write(byte0);
}

int32_t ReadInteger(WiFiClient& client) {
  int32_t byte1 = client.read();
  int32_t byte2 = client.read();
  int32_t byte3 = client.read();
  int32_t byte4 = client.read();
  return (byte1 << 24 | byte2 << 16 | byte3 << 8 | byte4);
}

void WriteInteger(WiFiClient& client, int32_t value) {
  uint8_t byte3 = (value >> 24) & 0xFF;
  uint8_t byte2 = (value >> 16) & 0xFF;
  uint8_t byte1 = (value >> 8 ) & 0xFF;
  uint8_t byte0 = (value 		) & 0xFF;
  client.write(byte3);
  client.write(byte2);
  client.write(byte1);
  client.write(byte0);
}

int64_t ReadLong(WiFiClient& client) {
  int64_t byte1 = client.read();
  int64_t byte2 = client.read();
  int64_t byte3 = client.read();
  int64_t byte4 = client.read();
  int64_t byte5 = client.read();
  int64_t byte6 = client.read();
  int64_t byte7 = client.read();
  int64_t byte8 = client.read();
  int64_t result = (
    byte1 << 56 |
    byte2 << 48 |
    byte3 << 40 |
    byte4 << 32 |
    byte5 << 24 |
    byte6 << 16 |
    byte7 <<  8 |
    byte8
  );
  return result;
}

void WriteLong(WiFiClient& client, int64_t value) {
  uint8_t byte7 = (value >> 56) & 0xFF;
  uint8_t byte6 = (value >> 48) & 0xFF;
  uint8_t byte5 = (value >> 40) & 0xFF;
  uint8_t byte4 = (value >> 32) & 0xFF;
  uint8_t byte3 = (value >> 24) & 0xFF;
  uint8_t byte2 = (value >> 16) & 0xFF;
  uint8_t byte1 = (value >> 8 ) & 0xFF;
  uint8_t byte0 = (value 		) & 0xFF;
  client.write(byte7);
  client.write(byte6);
  client.write(byte5);
  client.write(byte4);
  client.write(byte3);
  client.write(byte2);
  client.write(byte1);
  client.write(byte0);
}

float ReadFloat(WiFiClient& client) {
  uint32_t byte1 = client.read();
  uint32_t byte2 = client.read();
  uint32_t byte3 = client.read();
  uint32_t byte4 = client.read();
  uint32_t intBits = (
    byte4 << 24 |
    byte3 << 16 |
    byte2 <<  8 |
    byte1
  );
  float result;
  memcpy(&result, &intBits, sizeof(result));
  return result;
}

void WriteFloat(WiFiClient& client, float value) {
  uint32_t intValue;
  memcpy(&intValue, &value, sizeof(float));
  uint8_t byte3 = (intValue >> 24) & 0xFF;
  uint8_t byte2 = (intValue >> 16) & 0xFF;
  uint8_t byte1 = (intValue >> 8 ) & 0xFF;
  uint8_t byte0 = (intValue      ) & 0xFF;
  client.write(byte3);
  client.write(byte2);
  client.write(byte1);
  client.write(byte0);
}

double ReadDouble(WiFiClient& client) {
  uint64_t byte1 = client.read();
  uint64_t byte2 = client.read();
  uint64_t byte3 = client.read();
  uint64_t byte4 = client.read();
  uint64_t byte5 = client.read();
  uint64_t byte6 = client.read();
  uint64_t byte7 = client.read();
  uint64_t byte8 = client.read();
  uint64_t intBits = (
    byte1 << 56 |
    byte2 << 48 |
    byte3 << 40 |
    byte4 << 32 |
    byte5 << 24 |
    byte6 << 16 |
    byte7 <<  8 |
    byte8
  );
  double result;
  memcpy(&result, &intBits, sizeof(result));
  return result;
}

void WriteDouble(WiFiClient& client, double value) {
  uint64_t intValue;
  memcpy(&intValue, &value, sizeof(double));
  uint8_t byte7 = (intValue >> 56) & 0xFF;
  uint8_t byte6 = (intValue >> 48) & 0xFF;
  uint8_t byte5 = (intValue >> 40) & 0xFF;
  uint8_t byte4 = (intValue >> 32) & 0xFF;
  uint8_t byte3 = (intValue >> 24) & 0xFF;
  uint8_t byte2 = (intValue >> 16) & 0xFF;
  uint8_t byte1 = (intValue >> 8 ) & 0xFF;
  uint8_t byte0 = (intValue      ) & 0xFF;
  client.write(byte7);
  client.write(byte6);
  client.write(byte5);
  client.write(byte4);
  client.write(byte3);
  client.write(byte2);
  client.write(byte1);
  client.write(byte0);
}

char* ReadString16(WiFiClient& client) {
  int16_t length = ReadShort(client);
  char* buffer = (char*) malloc (length+1);
  for (int i = 0; i < length; i++) {
    // Read one byte since this is 16-Bit Character
    client.read();
    buffer[i] = client.read();
  }
  // Null terminator
  buffer[length] = 0;
  return buffer;
}

void WriteString16(WiFiClient& client, const char* message) {
  int16_t length = strlen(message);
  WriteShort(client, length);
  for (int16_t i = 0; i < length; i++) {
    // Read one byte since this is 16-Bit Character
    client.write((uint8_t)0);
    client.write(message[i]);
  }
}

void SendPreChunk(WiFiClient& client, int32_t x, int32_t z, bool mode) {
  WriteByte(client, PreChunk);
  WriteInteger(client, x);
  WriteInteger(client, z);
  WriteByte(client, mode);
}

void SendChunk(WiFiClient& client, int32_t x, int32_t z, int32_t compressedSize, const char* chunkData) {
  WriteByte(client,Chunk);
  WriteInteger(client, x);
  WriteShort(client, (int16_t)0);
  WriteInteger(client, z);
  WriteByte(client, (int8_t)15);
  WriteByte(client, (int8_t)127);
  WriteByte(client, (int8_t)15);
  WriteInteger(client, compressedSize);
  for (int32_t i = 0; i < compressedSize; i++) {
    WriteByte(client,(int8_t)chunkData[i]);
  }
}

void SendPlayerPosition(WiFiClient& client, struct Player& p) {
  WriteByte(client, PlayerPositionLook);
  WriteDouble(client, p.position.x);
  WriteDouble(client, p.position.y);
  WriteDouble(client, p.stance);
  WriteDouble(client, p.position.z);
  WriteFloat(client, p.yaw);
  WriteFloat(client, p.pitch);
  WriteByte(client, p.onGround);
}

void SendSpawnPosition(WiFiClient& client) {
  WriteByte(client, SpawnPosition);
  WriteInteger(client, spawnPoint.x);
  WriteInteger(client, spawnPoint.y);
  WriteInteger(client, spawnPoint.z);
}

void SendBlockChange(WiFiClient& client, Int3 pos, int8_t type, int8_t meta) {
  WriteByte(client, BlockChange);
  WriteInteger(client, pos.x);
  WriteByte(client, (int8_t)pos.y);
  WriteInteger(client, pos.z);
  WriteByte(client, type);
  WriteByte(client, meta);
}

void SendGlobalBlockChange(Int3 pos, int8_t type, int8_t meta) {
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    if (players[i].client && players[i].client.connected()) {
      SendBlockChange(players[i].client,pos,type,meta);
    }
  }
}

void SendSetSlot(WiFiClient& client, int8_t window, int16_t slot, int16_t id, int8_t amount, int16_t damage) {
  WriteByte(client, SetSlot);
  WriteByte(client, window);
  WriteShort(client, slot);
  WriteShort(client, id);
  WriteByte(client, amount);
  WriteShort(client, damage);
}

void SetPositionToSpawn(struct Player& p) {
  p.position.x = (double)spawnPoint.x;
  p.position.y = (double)spawnPoint.y+2.5; // Y position is the camera
  p.position.z = (double)spawnPoint.z;
}

void SendLoginRequest(WiFiClient& client, struct Player& p) {
  int32_t protocol = ReadInteger(client);
  if (protocol != 14) {
    Serial.println("Invalid Version");
  }
  ReadString16(client);
  ReadLong(client);
  ReadByte(client);

  // Reply to Login
  client.write(LoginRequest);
  WriteInteger(client,p.entityId);
  WriteString16(client,"");
  WriteLong(client,0);
  WriteByte(client,0);

  // Send spawn location
  SendSpawnPosition(client);
  SetPositionToSpawn(p);
  SendPlayerPosition(client,p);

  int32_t x = 0;
  int32_t z = 0;
  SendPreChunk(client, x, z,1);
  SendChunk(client, x, z,sizeof(compressedChunk),compressedChunk);

  // Set Spawn inventory
  SendSetSlot(client,0,INVENTORY_HOTBAR  , 1,1,0); // Stone
  SendSetSlot(client,0,INVENTORY_HOTBAR+1, 4,1,0); // Cobblestone
  SendSetSlot(client,0,INVENTORY_HOTBAR+2,45,1,0); // Bricks
  SendSetSlot(client,0,INVENTORY_HOTBAR+3, 3,1,0); // Dirt
  SendSetSlot(client,0,INVENTORY_HOTBAR+4, 5,1,0); // Planks
  SendSetSlot(client,0,INVENTORY_HOTBAR+5,17,1,0); // Logs
  SendSetSlot(client,0,INVENTORY_HOTBAR+6,18,1,0); // Leaves
  SendSetSlot(client,0,INVENTORY_HOTBAR+7,20,1,0); // Glass
  SendSetSlot(client,0,INVENTORY_HOTBAR+8,44,1,0); // Slab

  SendGlobalChatMessage("Joined",p.username);
}

void SendHandshake(WiFiClient& client, struct Player& p) {
  p.username = ReadString16(client);
  p.entityId = globalEntityId++;
  Serial.print(p.username);
  Serial.println(" has joined the game!");

  // Answer with a handshake packet
  client.write(Handshake);
  WriteString16(client,"-");
}

void SendPlayerBlockPlacement(WiFiClient& client, struct Player& p) {
  Int3 pos;
  pos.x = ReadInteger(client);
  pos.y = (int32_t)ReadByte(client);
  pos.z = ReadInteger(client);
  int8_t face = ReadByte(client);
  int16_t id = ReadShort(client);
  if (id > -1) {
    int8_t amount = ReadByte(client);
    int16_t damage = ReadShort(client);
    // Only change to valid block Ids
    if (id < 97) {
      FaceOffset(pos,face);
      SendGlobalBlockChange(pos,(int8_t)id,(int8_t)damage);
      // Give the placed item back to the player immediately
      SendSetSlot(client,0,INVENTORY_HOTBAR+p.hotbarSlot,id,1,damage);
    }
  }
}

void SendPlayerDigging(WiFiClient& client, struct Player& p) {
  int8_t status = ReadByte(client);
  Int3 pos;
  pos.x = ReadInteger(client);
  pos.y = (int32_t)ReadByte(client);
  pos.z = ReadInteger(client);
  int8_t face = ReadByte(client);
  // Break blocks instantly
  if (status == 0) {
    SendGlobalBlockChange(pos,0,0);
  }
}

void SendTransaction(WiFiClient& client, int8_t window, int16_t action, bool accepted) {
  WriteByte(client,window);
  WriteShort(client,action);
  WriteByte(client,accepted);
}

void SendChatMessage(WiFiClient& client, const char* sender, const char* message) {
  char* fullMessage = (char*)malloc(strlen(sender)+strlen(message)+3);
  if (fullMessage) {
    strcpy(fullMessage, "<");
    strcat(fullMessage, sender);
    strcat(fullMessage, "> ");
    strcat(fullMessage, message);
  }
  Serial.println(fullMessage);
  WriteByte(client, ChatMessage);
  WriteString16(client, fullMessage);
  free(fullMessage);
}
void SendGlobalChatMessage(const char* sender, const char* message) {
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    if (players[i].client && players[i].client.connected()) {
      SendChatMessage(players[i].client,sender,message);
    }
  }
}
void SendDisconnect(WiFiClient& client, const char* message) {
  WriteByte(client, Disconnect);
  WriteString16(client, message);
}

void ProcessClient(WiFiClient& client, struct Player& p) {
  if (client.available()) {
    char packetType = client.read();
    //Serial.print("Got Packet: 0x");
    //Serial.println(packetType,HEX);
    switch(packetType) {
      case LoginRequest:
        p.connectionStage = Login;
        SendLoginRequest(client,p);
        p.connectionStage = Connected;
        break;
      case Handshake:
        p.connectionStage = Shake;
        SendHandshake(client,p);
        break;
      case ChatMessage:
        {
        char* message = ReadString16(client);
        SendGlobalChatMessage(p.username,message);
        free(message);
        }
        break;
      case Player:
        p.onGround = ReadByte(client);
        break;
      case PlayerPosition:
        p.position.x = ReadDouble(client);
        p.position.y = ReadDouble(client);
        p.stance = ReadDouble(client);
        p.position.z = ReadDouble(client);
        p.onGround = ReadByte(client);
        break;
      case PlayerLook:
        p.yaw = ReadFloat(client);
        p.pitch = ReadFloat(client);
        p.onGround = ReadByte(client);
        break;
      case PlayerPositionLook:
        p.position.x = ReadDouble(client);
        p.position.y = ReadDouble(client);
        p.stance = ReadDouble(client);
        p.position.z = ReadDouble(client);
        p.yaw = ReadFloat(client);
        p.pitch = ReadFloat(client);
        p.onGround = ReadByte(client);
        break;
      case Animation:
        ReadInteger(client);
        ReadByte(client);
        break;
      case PlayerBlockPlacement:
        SendPlayerBlockPlacement(client,p);
        break;
      case PlayerDigging:
        SendPlayerDigging(client,p);
        break;
      case HoldingChange:
        p.hotbarSlot = ReadShort(client);
        break;
      case CloseWindow:
        ReadByte(client);
        break;
      case WindowClick:
        // Note: Moving stuff around in the inventory is not supported
        ReadByte(client);
        ReadShort(client);
        ReadByte(client);
        ReadShort(client);
        ReadByte(client);
        ReadShort(client);
        ReadByte(client);
        ReadShort(client);
        break;
      case EntityAction:
      {
        int32_t entityId = ReadInteger(client);
        int8_t action = ReadByte(client);
        break;
      }
      case Disconnect:
      {
        char* message = ReadString16(client);
        Serial.print(p.username);
        Serial.print(" has disconnected! (");
        Serial.print(message);
        Serial.println(")");
        free(message);
        p.connectionStage = Disconnected;
        break;
      }
      default:
        Serial.print("Unhandled Packet: 0x");
        Serial.println(packetType, HEX);
        WriteByte(client, KeepAlive);
        break;
    }
    if (p.connectionStage == Connected) {
      // Always send a keep alive packet
      WriteByte(client, KeepAlive);
      // Save the player from the void
      if (p.position.y < 0) {
        SetPositionToSpawn(p);
        SendPlayerPosition(client,p);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to open (optional for native USB)
  }

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient newClient = server.accept();

  if (newClient) {
    bool added = false;
    // Try to find an empty slot in the clients array
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
      if (!players[i].client || !players[i].client.connected()) {
        players[i].client = newClient;
        Serial.println("New Client Connected");
        players[i].connectionStage++;
        added = true;
        break;
      }
    }
    
    if (!added) {
      // If no space, you could reject the client or handle accordingly
      SendDisconnect(newClient,"Server is full!");
      newClient.stop();
      Serial.println("No space for new client, connection rejected");
    }
  }
  
  // Process each client
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    if (players[i].client && players[i].client.connected()) {
      // If the client is connected, process it
      ProcessClient(players[i].client, players[i]);
    } else if (players[i].client.connected()) {
      // Client has disconnected, stop and print message
      players[i].client.stop();
      //Serial.println("Client Disconnected");
    }
  }
}
