# Models Directory

This directory contains the AI models used by Zweek Code.

## Required Models

Place these GGUF models in this directory:

```
models/
├── smollm-135m-router.gguf    (Router: ~150MB)
├── Qwen3-0.6B-Q8_0.gguf       (Chat: ~700MB)
└── starcoder-tiny.gguf        (Code: ~200MB)
```

## Manual Download

Download GGUF quantized versions from Hugging Face:

1. **SmolLM-135M (Router)**: https://huggingface.co/HuggingFaceTB/SmolLM-135M-Instruct-GGUF
   - File: `smollm-135m-router.gguf`

2. **Qwen3-0.6B (Chat)**: https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF
   - File: `Qwen3-0.6B-Q8_0.gguf` (Q8_0 quantization)

3. **StarCoder-Tiny (Code)**: https://huggingface.co/bigcode/tiny_starcoder_py
   - File: `starcoder-tiny.gguf`

## Total Size

- **Full suite**: ~1.05GB
- **Minimal (router + code)**: ~350MB
- **Peak RAM during inference**: ~1.2GB

## Notes

- Models load on-demand to minimize memory usage
- Router stays resident in memory (~150MB) for instant classification
- Chat and code models unload after use (unless marked resident)
