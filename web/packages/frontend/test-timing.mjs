/**
 * Test script to measure backend query performance
 * Run this to see where the 6.5 seconds is being spent
 */

const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpZFVzdWFyaW8iOjE4MSwiaWRMb2phIjoyLCJ1c2VyIjoidG9ycmVzIiwidGlwbyI6IkFETUlOSVNUUkFET1IiLCJub21lIjoiUk9EUklHTyBUT1JSRVMiLCJpYXQiOjE3NjU0NjIwMDMsImV4cCI6MTc2NjA2NjgwM30.1jDEgaPb-R_IoWERjlFGebsZd_RsMjaQLdEUIPHbN80';

async function test() {
  console.log('\n╔═══════════════════════════════════════════════════╗');
  console.log('║ Backend Query Performance Test                    ║');
  console.log('╚═══════════════════════════════════════════════════╝\n');

  const metrics = {
    fetchStart: 0,
    fetchEnd: 0,
    parseStart: 0,
    parseEnd: 0,
    serializeStart: 0,
    serializeEnd: 0,
  };

  // Measure fetch + network
  console.log('Sending request to /trpc/orcamento.list...');
  metrics.fetchStart = performance.now();

  const response = await fetch('http://localhost:3001/trpc/orcamento.list?batch=1', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': `Bearer ${token}`,
    },
    body: JSON.stringify([
      {
        json: {
          statuses: ['ATIVO', 'EXPIRADO'],
          mesAno: '2025-01',
        },
      },
    ]),
  });

  metrics.fetchEnd = performance.now();
  console.log(`✓ Response received in ${(metrics.fetchEnd - metrics.fetchStart).toFixed(2)}ms\n`);

  // Measure text() conversion
  console.log('Converting response to text...');
  metrics.parseStart = performance.now();
  const text = await response.text();
  metrics.parseEnd = performance.now();
  console.log(`✓ Text conversion took ${(metrics.parseEnd - metrics.parseStart).toFixed(2)}ms`);
  console.log(`  Response size: ${(text.length / 1024 / 1024).toFixed(2)}MB\n`);

  // Measure JSON parsing
  console.log('Parsing JSON...');
  metrics.serializeStart = performance.now();
  const data = JSON.parse(text);
  metrics.serializeEnd = performance.now();
  console.log(`✓ JSON parsing took ${(metrics.serializeEnd - metrics.serializeStart).toFixed(2)}ms`);

  const orcamentos = data[0]?.result?.data || [];
  console.log(`  Parsed ${orcamentos.length} orcamentos\n`);

  // Summary
  console.log('╔═══════════════════════════════════════════════════╗');
  console.log('║ TIMING BREAKDOWN                                   ║');
  console.log('╚═══════════════════════════════════════════════════╝\n');

  const totalFetch = metrics.fetchEnd - metrics.fetchStart;
  const totalText = metrics.parseEnd - metrics.parseStart;
  const totalParse = metrics.serializeEnd - metrics.serializeStart;
  const totalTime = metrics.serializeEnd - metrics.fetchStart;

  console.log(`Network + Server Processing: ${totalFetch.toFixed(2)}ms`);
  console.log(`Text Conversion:             ${totalText.toFixed(2)}ms`);
  console.log(`JSON.parse():                ${totalParse.toFixed(2)}ms`);
  console.log(`─────────────────────────────────────────────────────`);
  console.log(`TOTAL:                       ${totalTime.toFixed(2)}ms\n`);

  if (totalFetch > 5000) {
    console.log('⚠️  NETWORK IS THE BOTTLENECK');
    console.log('   The server is slow to respond.');
    console.log('   Check:');
    console.log('   1. Database connection speed');
    console.log('   2. Database query execution time (use MySQL SLOW QUERY LOG)');
    console.log('   3. Network latency between server and database');
    console.log('   4. Whether indexes are being used properly\n');
  }

  if (totalParse > 1000) {
    console.log('⚠️  JSON PARSING IS SLOW');
    console.log('   Consider using MessagePack or reducing data size\n');
  }

  // Show query that was sent
  console.log('Query filters used:');
  console.log('  • statuses: ["ATIVO", "EXPIRADO"]');
  console.log('  • mesAno: "2025-01"');
  console.log('  • No search filter\n');

  console.log('Recommendation:');
  console.log('Check the backend console logs for "[Orcamento.list] Performance metrics"');
  console.log('This will show the breakdown: query build, query execute, normalization times');
}

test().catch(e => console.error('Test failed:', e));
