#include "audio_capture.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "pitch_tracker.h"
#ifdef SONARIA_MELODY_MODE
#include "melody_detector.h"
#define ACTIVE_CONFIRM_FRAMES MELODY_CONFIRM_FRAMES
static melody_detector_t melody;
#else
#define ACTIVE_CONFIRM_FRAMES PITCH_CONFIRM_FRAMES
#endif
#ifdef SONARIA_ENABLE_OLED
#include "tuner_display.h"
#endif
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define REPORT_EVERY 3
static const char *TAG = "pitch";
static int32_t rx_words[BLOCK_FRAMES][2];
static float frame[PITCH_FRAME_SIZE];
static pitch_tracker_t tracker;

_Static_assert(SAMPLE_RATE_HZ == PITCH_SAMPLE_RATE,
               "Audio and pitch sample rates must match");

static void report(tuner_result_t result, int64_t compute_us) {
  float dbfs = 20.0f * log10f(fmaxf(result.raw.rms, 1e-12f));
  char candidate[12] = "--";
  note_result_t note;
  if (result.candidate_frames && frequency_to_note(result.raw.frequency_hz, &note))
    snprintf(candidate, sizeof(candidate), "%s%d", note.name, note.octave);
  if (result.pitch.status == PITCH_VALID) {
    const char *tuning = result.tracking == TRACK_HOLD ? "HOLD"
                         : result.note.cents < -5 ? "FLAT"
                         : result.note.cents > 5 ? "SHARP" : "IN_TUNE";
    printf("status=%s frequency=%.2fHz note=%s%d cents=%+.2f confidence=%.3f "
           "dBFS=%.1f compute_us=%" PRId64 " tracking=%s candidate=%s count=%u/%d raw_confidence=%.3f\n",
           tuning, result.pitch.frequency_hz, result.note.name,
           result.note.octave, result.note.cents, result.pitch.confidence, dbfs,
           compute_us, pitch_tracking_name(result.tracking), candidate,
           result.candidate_frames, ACTIVE_CONFIRM_FRAMES, result.raw.confidence);
  } else {
    const char *status = result.tracking == TRACK_PENDING ? "PENDING"
                        : result.pitch.status == PITCH_SILENCE ? "SILENCE" : "UNCERTAIN";
    printf("status=%s frequency=-- note=-- cents=-- confidence=%.3f dBFS=%.1f "
           "compute_us=%" PRId64 " tracking=%s candidate=%s count=%u/%d raw_confidence=%.3f\n",
           status, result.pitch.confidence, dbfs, compute_us,
           pitch_tracking_name(result.tracking), candidate, result.candidate_frames,
           ACTIVE_CONFIRM_FRAMES, result.raw.confidence);
  }
}

void app_main(void) {
#ifdef SONARIA_ENABLE_OLED
  tuner_display_start();
#endif
#ifdef SONARIA_MELODY_MODE
  melody_detector_init(&melody);
#endif
  i2s_chan_handle_t rx = init_microphone();
#ifdef SONARIA_MELODY_MODE
  ESP_LOGI(TAG, "MELODY: %d Hz, frame=%d, hop=%d, MIDI range=%d..%d, confirm=%d",
           PITCH_SAMPLE_RATE,PITCH_FRAME_SIZE,PITCH_HOP_SIZE,MELODY_MIN_MIDI,MELODY_MAX_MIDI,MELODY_CONFIRM_FRAMES);
#else
  ESP_LOGI(TAG, "YIN: %d Hz, frame=%d, hop=%d, range=%.0f..%.0f Hz",
           PITCH_SAMPLE_RATE, PITCH_FRAME_SIZE, PITCH_HOP_SIZE, PITCH_MIN_HZ,
           PITCH_MAX_HZ);
#endif
  unsigned warmup = WARMUP_FRAMES;
  size_t filled = 0;
  unsigned report_count = 0;
  pitch_tracking_state_t previous_tracking = TRACK_IDLE;
  pitch_status_t previous_status = PITCH_SILENCE;
  int previous_midi = -1;
  int64_t max_compute_us = 0;
  while (1) {
    size_t bytes_read = 0;
    esp_err_t err =
        i2s_channel_read(rx, rx_words, sizeof(rx_words), &bytes_read, 1000);
    if (err != ESP_OK || bytes_read == 0 ||
        bytes_read % sizeof(rx_words[0]) != 0) {
      ESP_LOGW(TAG, "I2S read: %s bytes=%zu; resetting frame",
               esp_err_to_name(err), bytes_read);
      filled = 0;
      pitch_tracker_reset(&tracker);
#ifdef SONARIA_MELODY_MODE
      melody_detector_reset(&melody);
#endif
      previous_tracking = TRACK_IDLE;
      previous_status = PITCH_UNCERTAIN;
      previous_midi = -1;
      report_count = 0;
      max_compute_us = 0;
#ifdef SONARIA_ENABLE_OLED
      const tuner_result_t unavailable = {.pitch = {.status = PITCH_UNCERTAIN}, .raw = {.status = PITCH_UNCERTAIN}, .candidate_midi = -1};
      tuner_display_publish(&unavailable, 0);
#endif
      continue;
    }
    for (size_t i = 0; i < bytes_read / sizeof(rx_words[0]); ++i) {
      if (warmup > 0) {
        --warmup;
        continue;
      }
      frame[filled++] = pcm24(rx_words[i][0]) / (float)PCM24_SCALE;
      if (filled != PITCH_FRAME_SIZE)
        continue;
      int64_t start = esp_timer_get_time();
#ifdef SONARIA_MELODY_MODE
      tuner_result_t result = pitch_tracker_update_with_confirmation(&tracker, melody_detect(&melody, frame), MELODY_CONFIRM_FRAMES);
#else
      tuner_result_t result = pitch_track(&tracker, frame);
#endif
      int64_t elapsed = esp_timer_get_time() - start;
      if (elapsed > max_compute_us)
        max_compute_us = elapsed;
      int midi = result.pitch.status == PITCH_VALID ? result.note.midi : -1;
#ifdef SONARIA_ENABLE_OLED
      tuner_display_publish(&result, elapsed);
#endif
      if (++report_count >= REPORT_EVERY || result.tracking != previous_tracking ||
          result.pitch.status != previous_status || midi != previous_midi) {
        report(result, max_compute_us);
        report_count = 0;
        max_compute_us = 0;
      }
      previous_tracking = result.tracking;
      previous_status = result.pitch.status;
      previous_midi = midi;
      memmove(frame, frame + PITCH_HOP_SIZE,
              (PITCH_FRAME_SIZE - PITCH_HOP_SIZE) * sizeof(frame[0]));
      filled = PITCH_FRAME_SIZE - PITCH_HOP_SIZE;
    }
  }
}
