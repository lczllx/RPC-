#pragma once

/*区分消息类型
工作流程
注册阶段：为每种消息类型注册处理函数
接收消息：网络层收到消息，调用 Dispacher::onMessage
类型查找：根据 MsgType 找到对应的回调包装器
类型转换：将 BaseMessage 安全转换为具体类型
业务处理：调用具体的业务处理函数*/
#include "net.hpp"
#include "message.hpp"
#include "publicconfig.hpp"

namespace lcz_rpc
{
    class Callback
    {
        public:
        using ptr=std::shared_ptr<Callback>;
        virtual void onMessage(const BaseConnection::ptr& conn,BaseMessage::ptr& msg)=0;       
    };
    template<typename T>
    class CallbackType:public Callback
    {
        public:
        using ptr=std::shared_ptr<CallbackType<T>>;
        using MessageCallback=std::function<void (const BaseConnection::ptr& conn,std::shared_ptr<T>& msg)>;
        // // 支持右值引用的构造函数
        // CallbackType(MessageCallback&& handler) : _handler(std::move(handler)) {}
      
        CallbackType(const MessageCallback &handler):_handler(handler){}
        void onMessage(const BaseConnection::ptr& conn,BaseMessage::ptr& msg)
        {
            auto transmit_type=std::dynamic_pointer_cast<T>(msg);
            _handler(conn,transmit_type);// 调用具体类型的处理函数
        }
        private:
        MessageCallback _handler;
    };

    class Dispacher
    {
        public:
        using ptr=std::shared_ptr<Dispacher>;
        // //提供支持右值版本
        // template<typename T>
        // void registerhandler(MsgType msgtype,typename CallbackType<T>::MessageCallback&& handler)
        // {
        //     std::unique_lock<std::mutex> lock(_mutex);
        //     auto cb=std::make_shared<CallbackType<T>>(std::forward<typename CallbackType<T>::MessageCallback>(handler));// 创建类型特定的回调包装器
        //     _handlers.emplace(msgtype,cb);
        // }
         
        template<typename T>
        void registerhandler(MsgType msgtype,const typename CallbackType<T>::MessageCallback& handler)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto cb=std::make_shared<CallbackType<T>>(handler);// 创建类型特定的回调包装器
            _handlers.emplace(msgtype,cb);
        }
        void onMessage(const BaseConnection::ptr& conn,BaseMessage::ptr& msg)
        {
             if (!msg) {
                ELOG("收到空消息");
                return;
            }
            std::unique_lock<std::mutex> lock(_mutex);
            auto it=_handlers.find(msg->msgType());
            if(it!=_handlers.end())
            {
                return it->second->onMessage(conn,msg);// 调用对应的处理器
            }
            //没有找到指定类型的处理回调
            ELOG("收到未知消息类型 msgtype=%d (REQ_RPC=0, RSP_RPC=1, REQ_TOPIC=2, RSP_TOPIC=3, REQ_SERVICE=4, RSP_SERVICE=5)", 
                 static_cast<int>(msg->msgType()));
            conn->shutdown();//关闭未知消息的连接

        }
        private:
        std::mutex _mutex;
        std::unordered_map<MsgType,Callback::ptr>_handlers;
    };
}