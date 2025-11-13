# Sistemas-Operacionais---N2

## Descrição
Este repositório contém o código e a documentação para o **Trabalho N2 de Sistemas Operacionais**. O projeto aborda o monitoramento de processos e recursos do sistema, comunicação entre processos (IPC) usando FIFOs (named pipes), análise de comportamento do sistema de memória com alocação e gerenciamento de arquivos.

### Estrutura do repositório
A estrutura do repositório é organizada da seguinte forma:

- **`docs/`**: Contém o PDF do relatório final do trabalho.
- **`src/`**: Contém os arquivos de código fonte C para os exercícios realizados.
  - **`monitor.c`**: monitora o uso de CPU e memória dos processos em execução no sistema e alerta sobre processos que excedem limites de uso de CPU ou memória.
  - **`ipc_writer.c` e `ipc_reader.c`**: Códigos para comunicação entre processos (IPC) usando FIFO.
  - **`memhog_safe.c`**: simula o consumo de memória de forma controlada. Ele aloca blocos de memória de um tamanho especificado (em MB) até alcançar um total definido pelo usuário
  - **`gerenciamento.c`**: cria uma estrutura de diretórios organizada para um sistema de gerenciamento.
- **`logs/`**: Contém captura dos resultados dos testes.
- **`README.md`**: Este arquivo de descrição do projeto.

## Objetivos do Trabalho
1. **Gerenciamento de Arquivos e Permissões**:
   - Criar diretórios e arquivos, configurar permissões adequadas e listar arquivos modificados nas últimas 24h.
   
2. **Comunicação entre Processos (IPC)**:
   - Implementar comunicação entre processos usando **named pipes (FIFOs)**, com dois processos se comunicando via PING ↔ PONG.

3. **Memória e Alocação**:
   - Testar o comportamento do sistema operacional durante uma alocação intensiva de memória.

4. **Gerenciamento de Arquivos**:
   - Trabalhar com estrutura de diretórios, permissões e operações de
arquivos..
