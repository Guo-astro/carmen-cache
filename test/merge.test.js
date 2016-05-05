var Cache = require('../index.js').Cache;
var tape = require('tape');
var fs = require('fs');
var mp53 = Math.pow(2,53);

tape('#merge', function(assert) {
    var cacheA = new Cache('a');
    cacheA._set('term', 0, 1, [0,1,2,3]);
    cacheA._set('term', 0, 2, [0,1,2,3]);

    var cacheB = new Cache('b');
    cacheB._set('term', 0, 1, [10,11,12,13]);
    cacheB._set('term', 0, 3, [10,11,12,13]);

    var pbfA = cacheA.pack('term', 0);
    var pbfB = cacheB.pack('term', 0);

    var cacheC = new Cache('c');
    cacheA.merge(pbfA, pbfB, function(err, merged) {
        assert.ifError(err);
        cacheC.loadSync(merged, 'term', 0);
        assert.deepEqual(cacheC._get('term', 0, 2).sort(numSort), [0,1,2,3], 'a-only');
        assert.deepEqual(cacheC._get('term', 0, 3).sort(numSort), [10,11,12,13], 'b-only');
        assert.deepEqual(cacheC._get('term', 0, 1).sort(numSort), [0,1,2,3,10,11,12,13], 'a-b-merged');
        assert.end();
    })
});

function numSort(a, b) { return a - b; }
