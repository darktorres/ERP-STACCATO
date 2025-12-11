import { Module } from '@nestjs/common';
import { TrpcService } from './trpc.service.js';
import { TrpcRouter } from './trpc.router.js';
import { AuthModule } from '../modules/auth/auth.module.js';
import { OrcamentoModule } from '../modules/orcamento/orcamento.module.js';

@Module({
  imports: [AuthModule, OrcamentoModule],
  providers: [TrpcService, TrpcRouter],
  exports: [TrpcService, TrpcRouter],
})
export class TrpcModule {}
