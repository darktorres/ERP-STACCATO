import { Injectable, OnModuleInit, OnModuleDestroy } from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import { PrismaClient } from '@prisma/client';
import { PrismaMariaDb } from '@prisma/adapter-mariadb';

@Injectable()
export class PrismaService extends PrismaClient implements OnModuleInit, OnModuleDestroy {
  constructor(configService: ConfigService) {
    const connectionString = configService.get<string>('DATABASE_URL');
    if (!connectionString) {
      throw new Error('DATABASE_URL is not defined in environment variables');
    }

    const adapter = new PrismaMariaDb(connectionString);
    super({ adapter });
  }

  async onModuleInit() {
    // Attempt to connect with a timeout
    try {
      await Promise.race([
        this.$connect(),
        new Promise((_, reject) =>
          setTimeout(() => reject(new Error('Database connection timeout')), 5000)
        ),
      ]);
    } catch (error) {
      console.warn('Failed to connect to database on startup:', (error as Error).message);
      // Don't fail the entire app startup if database is unavailable
    }
  }

  async onModuleDestroy() {
    await this.$disconnect();
  }
}
