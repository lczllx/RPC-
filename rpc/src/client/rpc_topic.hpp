#pragma once
#include "requestor.hpp"
#include "caller.hpp"
#include "rpc_registry.hpp"
#include "../server/rpc_topic.hpp"
#include "../general/publicconfig.hpp"

namespace lcz_rpc
{
    namespace client
    {
        class TopicManager
        {
        public:
            using SubCallback = std::function<void(const std::string &, const std::string &)>;//消息推送调用的回调
            using ptr = std::shared_ptr<TopicManager>;
            TopicManager(const Requestor::ptr &requestor) : _requestor(requestor) {}
            bool createTopic(const BaseConnection::ptr &conn, const std::string &topic_name) { return commonRequest(conn, topic_name, TopicOpType::CREATE); }
            bool removeTopic(const BaseConnection::ptr &conn, const std::string &topic_name) { return commonRequest(conn, topic_name, TopicOpType::REMOVE); }
            bool subscribeTopic(const BaseConnection::ptr &conn, const std::string &topic_name, const SubCallback &cb,int priority=0,const std::vector<std::string> &tags={})
            {
                addSubscribe(topic_name,cb);
                bool ret=commonRequest(conn,topic_name,TopicOpType::SUBSCRIBE,"",TopicForwardStrategy::BROADCAST,0,"",priority,tags);
                if(!ret){
                    delSubscribe(topic_name);
                    return false;
                }
                return true;
            }
            bool cancelTopic(const BaseConnection::ptr &conn, const std::string &topic_name){return commonRequest(conn,topic_name,TopicOpType::UNSUBSCRIBE); }
            bool publishTopic(const BaseConnection::ptr &conn, const std::string &topic_name, const std::string &msg,
            TopicForwardStrategy strategy=TopicForwardStrategy::BROADCAST,int fanoutLimit=0,const std::string &shardKey="",
            int priority=0,const std::vector<std::string> &tags={}/*这样设置避免可能的悬挂引用*/,int redundantCount=0){       
                return commonRequest(conn,topic_name,TopicOpType::PUBLISH,msg,strategy,fanoutLimit,shardKey,priority,tags,redundantCount);}
            void onTopicPublish(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg)
            {
                auto type=msg->optype();
                if(type!=TopicOpType::PUBLISH)
                {
                    ELOG("接收到错误类型主题操作");
                    return;
                }
                std::string topic_name=msg->topicKey();
                std::string topic_msg=msg->topicMsg();
                auto pub_callback=getSubscribe(topic_name);
                if(!pub_callback)
                {
                    ELOG("接收到主题 %s 消息，但是没有对应回调",topic_name.c_str());
                    return;
                }
                return pub_callback(topic_name,topic_msg);
            }

        private:
            // 回调映射操作都需要持锁，防止并发订阅
            void addSubscribe(const std::string &topic_name, const SubCallback &cb)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _topic_cb[topic_name] = cb;
            }
            void delSubscribe(const std::string &topic_name)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _topic_cb.erase(topic_name);
            }
            const SubCallback getSubscribe(const std::string &topic_name)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _topic_cb.find(topic_name);
                if (it != _topic_cb.end())
                    return it->second;
                return SubCallback();
            }
            // 发送 TopicRequest 并同步等待 TopicResponse
            bool commonRequest(const BaseConnection::ptr &conn, const std::string &topic_name, TopicOpType op_type, const std::string &msg = "",
            TopicForwardStrategy strategy=TopicForwardStrategy::BROADCAST,int fanoutLimit=0,const std::string &shardKey="",
            int priority=0,const std::vector<std::string> &tags={},int redundantCount=0)
            {
                TopicRequest::ptr req_msg = MessageFactory::create<TopicRequest>();
                req_msg->setId(uuid());
                req_msg->setMsgType(MsgType::REQ_TOPIC);
                req_msg->setTopicKey(topic_name);
                req_msg->setOptype(op_type);
               
                if (op_type == TopicOpType::PUBLISH)
                {
                    req_msg->setTopicMsg(msg);
                    req_msg->setForwardStrategy(strategy);
                    // 设置消息路由控制参数（仅当参数有效时设置）
                    if(fanoutLimit>0)req_msg->setFanoutLimit(fanoutLimit);
                    if(!shardKey.empty())req_msg->setShardKey(shardKey);
                    if(priority>0)req_msg->setPriority(priority);
                    if(!tags.empty())req_msg->setTags(tags);
                    if(redundantCount>1)req_msg->setRedundantCount(redundantCount);
                }
                //订阅消息需要设置优先级和标签
                if (op_type == TopicOpType::SUBSCRIBE)
                {
                    if (priority > 0) req_msg->setPriority(priority);
                    if (!tags.empty()) req_msg->setTags(tags);
                }
                BaseMessage::ptr resp_msg;
                bool ret = _requestor->send(conn, std::dynamic_pointer_cast<BaseMessage>(req_msg), resp_msg);
                if (!ret)
                {
                    ELOG("topic请求失败");
                    return false;
                }
                TopicResponse::ptr topic_respmsg = std::dynamic_pointer_cast<TopicResponse>(resp_msg);
                if (topic_respmsg.get() == nullptr)
                {
                    ELOG("topic类型向下转换失败失败");
                    return false;
                }
                if (topic_respmsg->rcode() != RespCode::SUCCESS)
                {
                    ELOG("topic请求出错：%s", errReason(topic_respmsg->rcode()).c_str());
                    return false;
                }
                return true;
            }

        private:
            std::mutex _mutex;
            std::unordered_map<std::string, SubCallback> _topic_cb;//保存一个主题对应的回调
            Requestor::ptr _requestor; // 对请求发送，响应的接收处理
        };

    } // namespace client
}