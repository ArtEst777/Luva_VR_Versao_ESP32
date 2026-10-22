# Documentação Técnica de Engenharia — Projeto L-UVA

Este diretório concentra os relatórios técnicos, especificações de bancada, modelagem matemática e protocolos de validação desenvolvidos pela equipe do **Laboratório de Visualização, Interação e Simulação (L-VIS)** da **Universidade de Brasília (UnB)**.

---

## Índice de Documentos

1. **[Especificação de Hardware, Soldagem, Manufatura e Validação Elétrica (`hardware-e-soldagem.md`)](hardware-e-soldagem.md)**
   - Arquitetura funcional em placa universal, condicionamento de sensores flexíveis (divisores resistivos), mapa completo de pinos e *netlist*, usinagem de base em acrílico, costura técnica na luva, protocolo de inspeção a frio/quente e diagnóstico de falhas.
2. **[Arquitetura de Simulação 3D, Cinemática Articular e Processamento de Sinais (`arquitetura-simulacao.md`)](arquitetura-simulacao.md)**
   - Modelagem matemática da rotação de ossos na Unity ($\text{NMF}/\text{ES} \times \text{VFL} + \text{VRR}$), lógica de preensão (*Grab System*), filtragem digital Butterworth de 2ª ordem e fusão sensorial por Filtro de Kalman de 2 estados.
