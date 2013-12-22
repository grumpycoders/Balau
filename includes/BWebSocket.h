#pragma once

#include <Task.h>
#include <StacklessTask.h>
#include <BStream.h>
#include <HttpServer.h>

namespace Balau {

class WebSocketActionBase;

class WebSocketWorker : public StacklessTask {
  public:
    virtual bool parse(Http::Request & req) { return true; }
  protected:
      WebSocketWorker(IO<Handle> socket, const String & url) : m_socket(new BStream(socket)) { m_name = String("WebSocket:") + url + "/" + m_socket->getName(); }
      ~WebSocketWorker() { free(m_payload); }
    void disconnect() { m_socket->close(); }
    virtual void receiveMessage(const uint8_t * msg, size_t len, bool binary) = 0;
private:
    void processMessage();
    void processPing();
    void processPong();
    const char * getName() const { return m_name.to_charp(); }
    void Do();
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
