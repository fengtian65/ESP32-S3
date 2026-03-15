import json
import requests

# ========== 替换为你的配置 ==========
COZE_TOKEN = "eyJhbGciOiJSUzI1NiIsImtpZCI6Ijg4OTdjOGE0LTc5ZDEtNDI0My04ZmZmLTRhNjQzNDgwYjRhNCJ9.eyJpc3MiOiJodHRwczovL2FwaS5jb3plLmNuIiwiYXVkIjpbInJLSFJuNmo3Q3FQZmtKVkJGOWg5SWFCVzNNWkluNkY1Il0sImV4cCI6ODIxMDI2Njg3Njc5OSwiaWF0IjoxNzczMzEyNjQ4LCJzdWIiOiJzcGlmZmU6Ly9hcGkuY296ZS5jbi93b3JrbG9hZF9pZGVudGl0eS9pZDo3NjE2MzE3MDYxNDIxMDcyNDAzIiwic3JjIjoiaW5ib3VuZF9hdXRoX2FjY2Vzc190b2tlbl9pZDo3NjE2MzE5ODI5NDQzNjA4NjI2In0.AuVW5O8hwAHRK8pK438a_JHSMZTbfmemntjpoKopKVn6LZZeoxaWRe7helMaHkFnpXd6Woiw4e9YUobaS0V94kPp9TbBhYxns1a24QeKUfg3pdRLrHF5vojAdonVGZ_PuuzZPyydpeRWKcTG53y6PQVmOCufyejNuZjVHR_4xPU1hUldmssc08XYfg7ZuRDgm_l1scSESqcLEUFVzT2Nds9Nqgw5n_3ZqVVo7PorNQCzQU61s61s8gxYy2dRMzW1ApfRent2J-ktze_VogYwum4o1RRJtDGDWctDWgh5pmX0_dhZnknxlifhGdDlFIpUzXhLJYmVBqdjg23ffWAWHA"
COZE_URL = "https://pcgrcsptjq.coze.site/stream_run"
COZE_SESSION = "ESP32_S3_CUSTOM_SESSION_001"  # 自定义/界面生成的都可以
COZE_PROJECT = "7616311002971521087"
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