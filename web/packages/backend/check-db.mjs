import { PrismaClient } from '@prisma/client';

const prisma = new PrismaClient();

async function checkData() {
  try {
    const count = await prisma.orcamento.count();
    console.log(`Database has ${count} orcamentos`);
    
    if (count > 0) {
      const first = await prisma.orcamento.findFirst({
        include: {
          loja: true,
          vendedor: true,
        }
      });
      console.log('\nFirst orcamento:');
      console.log('- idOrcamento:', first.idOrcamento);
      console.log('- status:', first.status);
      console.log('- loja:', first.loja?.descricao);
      console.log('- vendedor:', first.vendedor?.nome);
      console.log('- total:', first.total);
      console.log('- data:', first.data);
    }
  } catch (error) {
    console.error('Error:', error.message);
  } finally {
    await prisma.$disconnect();
  }
}

checkData();
