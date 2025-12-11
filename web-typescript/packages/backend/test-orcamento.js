const fetch = require('node-fetch');

async function testOrcamentoApi() {
  try {
    // Test 1: List all orcamentos without filters
    console.log('\n=== Test 1: List all orcamentos ===');
    const response = await fetch('http://localhost:3001/trpc/orcamento.list', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Authorization': 'Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpZFVzdWFyaW8iOjEsInRpcG8iOiJBRE1JTklTVFJBRE9SIiwiaWRMb2phIjoxfQ.test'
      },
      body: JSON.stringify({
        jsonrpc: '2.0',
        id: 1,
        method: 'orcamento.list',
        params: { input: {} }
      })
    });

    const data = await response.json();
    console.log('Response status:', response.status);
    console.log('Response:', JSON.stringify(data, null, 2));

    if (data.result) {
      console.log(`\nFound ${data.result.length} orcamentos`);
      if (data.result.length > 0) {
        console.log('First orcamento:', JSON.stringify(data.result[0], null, 2));
      }
    }

    // Test 2: List with status filter
    console.log('\n=== Test 2: List with ATIVO status filter ===');
    const response2 = await fetch('http://localhost:3001/trpc/orcamento.list', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Authorization': 'Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpZFVzdWFyaW8iOjEsInRpcG8iOiJBRE1JTklTVFJBRE9SIiwiaWRMb2phIjoxfQ.test'
      },
      body: JSON.stringify({
        jsonrpc: '2.0',
        id: 2,
        method: 'orcamento.list',
        params: { input: { statuses: ['ATIVO'] } }
      })
    });

    const data2 = await response2.json();
    console.log('Response status:', response2.status);
    if (data2.result) {
      console.log(`Found ${data2.result.length} ATIVO orcamentos`);
    } else if (data2.error) {
      console.log('Error:', data2.error);
    }

  } catch (error) {
    console.error('Test failed:', error);
  }

  process.exit(0);
}

testOrcamentoApi();
