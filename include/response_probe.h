#ifndef RESPONSE_PROBE_H
#define RESPONSE_PROBE_H
/* trykernel.h、knldef.hの後に読み込む。対象とvalidの操作は割込み禁止下で行う。 */
extern TCB *response_probe_task;
extern volatile UW response_probe_ready_us;
extern volatile BOOL response_probe_valid;
extern volatile UW response_flash_sequence;
extern volatile UW response_flash_start_us;
extern volatile UW response_flash_end_us;
extern volatile UW response_ready_flash_sequence;
#endif
