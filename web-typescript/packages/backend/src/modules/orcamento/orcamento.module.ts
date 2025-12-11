import { Module } from '@nestjs/common';
import { OrcamentoService } from './orcamento.service.js';

@Module({
  providers: [OrcamentoService],
  exports: [OrcamentoService],
})
export class OrcamentoModule {}
