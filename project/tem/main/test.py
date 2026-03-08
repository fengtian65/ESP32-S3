import json
import requests

# ========== 替换为你的配置 ==========
COZE_TOKEN = "eyJhbGciOiJSUzI1NiIsImtpZCI6IjIzMjMwOGIyLWJkMTMtNGE4Mi1iZWJmLTcxMWZiMTQ2NThiOCJ9.eyJpc3MiOiJodHRwczovL2FwaS5jb3plLmNuIiwiYXVkIjpbIlZSa3NUUGoxMHpxa2lqUlVzY3M4YlRiRG1qblFrd2hhIl0sImV4cCI6ODIxMDI2Njg3Njc5OSwiaWF0IjoxNzcyOTM0MDI3LCJzdWIiOiJzcGlmZmU6Ly9hcGkuY296ZS5jbi93b3JrbG9hZF9pZGVudGl0eS9pZDo3NjEwMjQ4MDMyNzg0NzQ0NDgzIiwic3JjIjoiaW5ib3VuZF9hdXRoX2FjY2Vzc190b2tlbl9pZDo3NjE0NjkzNjY1NDAyNzgxNzM5In0.gbFGfHR09me1E2wTQ_yVaMrTXfNmGlWE6mBRzgEixlZj7M-HVhrVLoopZ9W-YWmxekwYj9YF4B-KOojD7hYjqAXeGJqIbN8A-tlsQt36xZA18ka2_Mjb0gVQfMEVuSpN1EyA4pjjxlPT1v64VbXKGjYEI2qlZKqe-YUXeoLttN7WWAWinQ29T1KffUmgT1Dv38Jf9Cg7X_KqgwLa4qDin8rCNhOgtdNvm9uJ484jB8FTVX9Z2TEiuwANWwbGISNMdw2-oCYiNHI0taTsUMfQufTqqwByBtuxTITCk9tyxqsGqlTvEdKwTvxyGIdRn6cgxwdvtRT5nTkJLqCohkKkwQ"
COZE_URL = "https://4npkk23hhg.coze.site/stream_run"
COZE_SESSION = "ESP32_S3_CUSTOM_SESSION_001"  # 自定义/界面生成的都可以
COZE_PROJECT = "7610242979017719860"
# ===================================

# 构建请求头
headers = {
    "Authorization": f"Bearer {COZE_TOKEN}",
    "Content-Type": "application/json",
    "Accept": "text/event-stream",
    "Cache-Control": "no-cache",
    "Connection": "keep-alive"
}

# 构建请求体
payload = {
    "content": {
        "query": {
            "prompt": [
                {
                    "type": "text",
                    "content": {
                        "text": "温度:21.0℃,湿度:21.0%。请给出生活建议。"
                    }
                }
            ]
        }
    },
    "type": "query",
    "session_id": COZE_SESSION,
    "project_id": COZE_PROJECT
}

# 发送流式请求
try:
    print("开始发送请求到Coze API...")
    response = requests.post(
        COZE_URL,
        headers=headers,
        json=payload,
        stream=True,
        timeout=60
    )
    print(f"HTTP响应码: {response.status_code}")
    
    if response.status_code != 200:
        print(f"❌ 服务端返回错误：{response.text}")
        exit(1)
    
    # 关键：拼接完整回复
    full_answer = ""  # 存储完整回复的缓冲区
    print("\n========== Coze智能体完整回复 ==========")
    
    for line in response.iter_lines(decode_unicode=True):
        if line:
            # 只处理data行，忽略event行
            if line.startswith("data:"):
                data_text = line[5:].strip()
                if data_text == "[DONE]":
                    break
                # 解析JSON并拼接answer
                try:
                    parsed = json.loads(data_text)
                    # 提取content.answer字段
                    if "content" in parsed and "answer" in parsed["content"]:
                        answer_char = parsed["content"]["answer"]
                        full_answer += answer_char  # 逐字拼接
                    # 检查是否回复结束
                    if parsed.get("finish", False):
                        break
                except:
                    pass  # 忽略非JSON格式的行
    
    # 输出完整回复
    print(full_answer)
    print("========== 回复结束 ==========")

except Exception as e:
    print(f"❌ 请求失败：{str(e)}")