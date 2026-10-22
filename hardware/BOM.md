# Bill of Materials (BOM) — Lista de Materiais e Estimativa de Custo

Um dos principais diferenciais competitivos da **L-UVA** perante luvas hápticas e de captura de movimento comerciais (cujo custo frequentemente ultrapassa milhares de dólares) é a sua **arquitetura de baixo custo baseada em componentes acessíveis no mercado nacional**.

---

## 1. Componentes Eletrônicos (Unidade por Mão)

| Item | Qtd. | Componente / Especificação | Função no Sistema | Custo Estimado (R\$) |
| :---: | :---: | :--- | :--- | :---: |
| 1 | 1 | **ESP32 DevKit V1** (30 pinos, Dual-Core) | Aquisição ADC 12-bit, filtragem digital e comunicação USB/Serial | R\$ 35,00 – 45,00 |
| 2 | 1 | **Módulo IMU MPU6050 (GY-521)** (8 pinos, 6-DoF) | Acelerômetro triaxial + Giroscópio triaxial ($\text{I}^2\text{C}$) | R\$ 25,00 – 35,00 |
| 3 | 4 a 5 | **Sensores Flexíveis Resistivos** (2.2" ou 4.5") | Medição contínua da flexão articular dos dedos | R\$ 120,00 – 200,00 |
| 4 | 4 a 5 | **Resistores de Filme Metálico $10\text{ k}\Omega$** ($1/4\text{ W}$, $1\%$) | Parte fixa dos divisores resistivos de tensão | R\$ 1,00 |
| 5 | 1 | **Placa Universal Ilhada com Barras Laterais** | Suporte eletromecânico e barramentos `3V3`/`GND` | R\$ 10,00 – 15,00 |
| 6 | 4 a 5 | **Conectores Modulares de 2 Vias** (JST / KK / Borne) | Conexão removível dos sensores flexíveis | R\$ 5,00 |
| 7 | 1 | **Cabo Micro-USB / USB-C de Dados** | Alimentação e link de telemetria com a estação VR | R\$ 15,00 |

---

## 2. Materiais Estruturais e Têxteis (Manufatura Vestível)

| Item | Especificação | Aplicação na Montagem |
| :---: | :--- | :--- |
| 1 | **Luva têxtil ergonômica** | Base vestível do protótipo |
| 2 | **Chapa de acrílico usinada** | Interface estrutural entre o dorso têxtil da luva e a placa do circuito |
| 3 | **Fitas/Bolsinhos de elástico** | Alojamento guiado dos sensores flex ao longo dos dedos |
| 4 | **Fio de algodão de alta resistência + Cola de selagem** | Fixação por amarração cirúrgica da base de acrílico e reforço dos bolsinhos |
| 5 | **Fios isolados (AWG 24–28), Solda, Fluxo e Termo-retrátil** | Interligação das redes elétricas e alívio de tração mecânica |

---

## 3. Ferramental de Bancada Utilizado no L-VIS

- Estação de solda com controle de temperatura ($320\ ^\circ\text{C}$ a $380\ ^\circ\text{C}$) e ponta fina;
- Multímetro digital (inspeção de continuidade, resistência e tensão de barramento);
- Micro retífica (Dremel) equipada com disco de corte de metal e fresa de alta velocidade (usinagem do acrílico);
- Agulhas grossas (reta e curvada), pinça de precisão, suporte de placa e lupa de bancada;
- Álcool isopropílico, escova antiestática e fita Kapton.
