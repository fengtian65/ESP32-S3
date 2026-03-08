#ifndef COZE_API_H
#define COZE_API_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 向 Coze 智能体发送温湿度数据，获取生活建议并打印
 * @param temp 温度（℃）
 * @param humid 湿度（%）
 */
void coze_send_request(float temp, float humid);

#ifdef __cplusplus
}
#endif

#endif // COZE_API_H