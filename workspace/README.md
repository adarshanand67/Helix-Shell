# HelixShell Workspace

This directory is mounted as a persistent volume when running HelixShell in Docker.

Files created here will persist across container restarts.

## Example Usage

```bash
docker run -it --rm -v $(pwd)/workspace:/home/shelluser/workspace helixshell:latest
```

