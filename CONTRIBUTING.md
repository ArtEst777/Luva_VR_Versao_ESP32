# Guia de Contribuição e Padrões de Engenharia — Projeto L-UVA

Bem-vindo ao repositório oficial da **L-UVA**, a luva de realidade virtual e captura de dados de baixo custo desenvolvida no **Laboratório de Visualização, Interação e Simulação (L-VIS)** da **Universidade de Brasília (UnB)**.

Este documento define as diretrizes para colaboradores acadêmicos, pesquisadores e parceiros externos que desejam contribuir com o desenvolvimento de **Hardware**, **Firmware** ou **Simulação (Unity 3D)**.

---

## 1. Estrutura de Branches e Fluxo de Trabalho

Utilizamos um fluxo de trabalho baseado em *Feature Branches* para manter a branch `main` sempre estável e pronta para demonstrações executivas e testes integrados:

- `main`: Código de produção validado em bancada e homologado pela gestão do projeto.
- `feat/<escopo>-<descricao>`: Novas funcionalidades (ex.: `feat/unity-grab-physics`, `feat/firmware-ble-serial`).
- `fix/<escopo>-<descricao>`: Correções de bugs ou ajustes de calibração (ex.: `fix/kalman-drift-roll`).
- `docs/<descricao>`: Atualizações de relatórios técnicos, diagramas ou documentação executiva.

---

## 2. Padrões por Frente de Desenvolvimento

### 2.1. Firmware Embarcado (`/firmware`)
- **Plataforma Alvo**: ESP32 DevKit V1 (arquitetura Dual-Core Xtensa LX6).
- **Tempo Real**: Tarefas críticas de amostragem (`TaskLeituraFlex` e `TaskLeituraMPU`) devem utilizar primitivos do FreeRTOS (`vTaskDelayUntil`, `xSemaphoreCreateMutex`) evitando bloqueios (`delay()` convencional) no loop de aquisição.
- **Processamento de Sinais**: Qualquer alteração nas frequências de amostragem ($f_s$) exige o recálculo dos coeficientes de filtros digitais (Butterworth) ou das matrizes de covariância ($Q$ e $R$) do Filtro de Kalman.

### 2.2. Hardware e Eletrônica (`/hardware`)
- **Nível Lógico**: Todo o barramento de sensores opera estritamente em **$3,3\text{ V}$**. Nunca conectar sinais de $5\text{ V}$ (`VIN`) aos pinos ADC ou ao barramento $\text{I}^2\text{C}$ da ESP32.
- **Validação Incremental**: Novas revisões de placa universal ou PCB devem passar integralmente pelo checklist de inspeção a frio (continuidade e isolamento de barramentos) antes da primeira energização via USB.

### 2.3. Simulação Virtual e Unity (`/unity`)
- **Desacoplamento**: Os scripts de recebimento de dados seriais devem permanecer desacoplados dos scripts de cinemática articular, comunicando-se por meio das variáveis padronizadas de estado (`ES` e `VFL`).
- **Controle de Versão**: Certifique-se de que as configurações do projeto Unity estejam com `Visible Meta Files` e `Force Text` ativados para versionamento limpo no Git.

---

## 3. Submissão de Pull Requests (PRs)

1. Descreva claramente o **objetivo técnico** da alteração.
2. Indique se a modificação foi **validada fisicamente no protótipo** ou em bancada de simulação.
3. Anexe gráficos do *Serial Plotter*, capturas de tela da Unity ou métricas de bancada quando aplicável.
