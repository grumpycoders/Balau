#pragma once

#include <Task.h>
#include <StacklessTask.h>
#include <BStream.h>
#include <HttpServer.h>

namespace Balau {

class WebSocketActionBase;

class WebSocketFrame {
  public:
      WebSocketFrame(const String & str, uint8_t opcode, bool mask = false) : WebSocketFrame((uint8_t *) str.to_charp(), str.strlen(), opcode, mask) { }
      WebSocketFrame(size_t len, uint8_t opcode, bool mask = false) : WebSocketFrame(NULL, len, opcode, mask) { }
      WebSocketFrame(const uint8_t * data, size_t len, uint8_t opcode, bool mask = false);
      ~WebSocketFrame() { free(m_data); }
    uint8_t & operator[](size_t idx);
    uint8_t * getPtr() { return m_data + m_headerSize; }
    void send(IO<Handle> socket);
  private:
    uint8_t * m_data = NULL;
    size_t m_len = 0;
    size_t m_headerSize = 0;
    uint32_t m_mask = 'BLAH';
    size_t m_bytesSent = 0;
};

class WebSocketWorker : public StacklessTask {
  public:
    virtual bool parse(Http::Request & req) { return true; }
    void sendFrame(WebSocketFrame * frame) { m_sendQueue.push(frame); }
    void enforceServer(void) throw (GeneralException);
    void enforceClient(void) throw (GeneralException);
  protected:
      WebSocketWorker(IO<Handle> socket, const String & url) : m_socket(new BStream(socket)) { m_name = String("WebSocket:") + url + ":" + m_socket->getName(); }
      ~WebSocketWorker();
    void disconnect() { m_socket->close(); }
    virtual void receiveMessage(const uint8_t * msg, size_t len, bool binary) = 0;
    virtual void Do();
private:
    void processMessage();
    void processPing();
    void processPong();
    virtual const char * getName() const { return m_name.to_charp(); }
    String m_name;
    IO<BStream> m_socket;
    enum {
        READ_H,
        READ_PLB,
        READ_PLL,
        READ_MK,
        READ_PL,
    } m_status = READ_H;
    enum { MAX_WEBSOCKET_LIMIT = 4 * 1024 * 1024 };
    uint8_t * m_payload = NULL;
    WebSocketFrame * m_sending = NULL;
    TQueue<WebSocketFrame> m_sendQueue;
    uint64_t m_payloadLen;
    uint64_t m_totalLen;
    uint64_t m_remainingBytes;
    uint32_t m_mask;
    uint8_t m_opcode;
    bool m_hasMask;
    bool m_fin;
    bool m_firstFragment = true;
    bool m_enforceServer = false;
    bool m_enforceClient = false;
    enum {
        OPCODE_CONT  =  0,
        OPCODE_TEXT  =  1,
        OPCODE_BIN   =  2,
        OPCODE_CLOSE =  8,
        OPCODE_PING  =  9,
        OPCODE_PONG  = 10,
    };
    friend class WebSocketActionBase;
};

class WebSocketServerBase : public HttpServer::Action {
  protected:
      WebSocketServerBase(const Regex & regex) : Action(regex) { }
    virtual WebSocketWorker * spawnWorker(IO<Handle> socket, const String & url) = 0;
  private:
    void sendError(IO<Handle> out, const char * serverName);
    bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
};

template<class T>
class WebSocketServer : public WebSocketServerBase {
  protected:
      WebSocketServer(const Regex & regex) : WebSocketServerBase(regex) { }
    virtual WebSocketWorker * spawnWorker(IO<Handle> socket, const String & url) { return new T(socket, url); }
};

};
